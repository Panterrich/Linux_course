#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>

#define OR_DIE do {if (result) return result;} while(0)
struct Sended_file 
{
    pid_t receiver_pid;
    siginfo_t info;

    int    fd;
    char*  file_path;

    char*  buffer;
    size_t size;
    size_t index;
};

enum ERRORS
{
    ERROR_INVALID_ARGC   = -1,
    ERROR_FILE_OPEN      = -2,
    ERROR_MMAP           = -3,
    ERROR_UNMAP          = -4,
    ERROR_SIGPROCMASK    = -5,
    ERROR_SIGQUEUE       = -6
};

size_t size_file(int fd);

int open_mmap_file(struct Sended_file* file, char* name_file);

int sigconfigure();

int send_size(struct Sended_file* file, sigset_t* set);
int send_data(struct Sended_file* file, sigset_t* set);

void handler_TERM(int sig);
void handler_INT(int sig);

struct Sended_file snd_file = {};

int main(int argc, char* argv[])
{   
    struct timeval start = {}, end = {};
    struct timeval start_transmission = {}, end_transmission = {};
    gettimeofday(&start, NULL);

    if (argc != 3) 
    {
        printf("Please, enter 1 src file path and PID receiver!\n");
        return ERROR_INVALID_ARGC;
    }

    sigset_t set = {};

    int result = sigconfigure(&set); OR_DIE;
        result = open_mmap_file(&snd_file, argv[1]);
    if (result)
    {
        if (munmap(snd_file.buffer, snd_file.size) == -1)
        {
            perror("ERROR: dst munmap");
        }
        return result;
    }

    snd_file.receiver_pid = strtol(argv[2], &argv[2], 10);


    gettimeofday(&start_transmission, NULL);
    if ((result = send_size(&snd_file, &set)) != 0)
    {   
        if (munmap(snd_file.buffer, snd_file.size) == -1)
        {
            perror("ERROR: src munmap");
        }
        return result;
    }
    
    if ((result = send_data(&snd_file, &set)) != 0)
    {
        if (munmap(snd_file.buffer, snd_file.size) == -1)
        {
            perror("ERROR: src munmap");
        }
        return result;
    }

    if (kill(snd_file.receiver_pid, SIGTERM) == -1) handler_TERM(SIGTERM);
    gettimeofday(&end_transmission, NULL);


    if (munmap(snd_file.buffer, snd_file.size) == -1)
    {
        perror("ERROR: src munmap");
        return  ERROR_UNMAP;
    }

    gettimeofday(&end, NULL);
    
    printf("Only transmition time:  %lg mb/sec\n",  1.0 * snd_file.size / 1024 / 1024 / (end_transmission.tv_sec - start_transmission.tv_sec));
    printf("Total transmition time: %lg mb/sec\n",  1.0 * snd_file.size / 1024 / 1024 / (end.tv_sec - start.tv_sec));
    return 0;
}

size_t size_file(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}

int send_size(struct Sended_file* file, sigset_t* set)
{   
    assert(file);

    union sigval value = {file->size};
    int result = sigqueue(file->receiver_pid, SIGUSR1, value);
    if (result == -1)
    {
        perror("ERROR: sigqueue SIGUSR1");
        return ERROR_SIGQUEUE;
    }
    
    int sig = 0;
    while (!(file->info.si_signo == SIGUSR1 && file->info.si_pid == file->receiver_pid))
    {
        sig = sigwaitinfo(set, &file->info);
        if (sig == SIGTERM) handler_TERM(sig);
    }
    
    return 0;
}

int send_data(struct Sended_file* file, sigset_t* set)
{   
    assert(file);

    int          sig   = 0;
    size_t       index = 0;
    union sigval value = {};

    while (index + 7 < file->size)
    {  
        memcpy(&value, file->buffer + index, sizeof(void*));
        if (sigqueue(file->receiver_pid, SIGUSR2, value) == -1)
        {
            perror("ERROR: sigqueue SIGUSR2");
            return ERROR_SIGQUEUE;
        }

        index += sizeof(void*);
        value.sival_ptr = NULL;

        do
        {       
            memset(&file->info, 0, sizeof(siginfo_t));   
            sig = sigwaitinfo(set, &file->info);
            if (sig == SIGTERM) handler_TERM(sig);

        } while (!(file->info.si_signo == SIGUSR2 && file->info.si_pid == file->receiver_pid));
        
    }

   
    memcpy(&value, file->buffer + index, file->size - index);
    if (sigqueue(file->receiver_pid, SIGUSR2, value) == -1)
    {
        perror("ERROR: sigqueue SIGUSR2");
        return ERROR_SIGQUEUE;
    }

    
    do
    {   
        memset(&file->info, 0, sizeof(siginfo_t));
        sig = sigwaitinfo(set, &file->info);
        if (sig == SIGTERM) handler_TERM(sig);

    } while (!(file->info.si_signo == SIGUSR2 && file->info.si_pid == file->receiver_pid));

    return 0;
}

int open_mmap_file(struct Sended_file* file, char* name_file)
{
    assert(file);
    assert(name_file);

    file->file_path = name_file;
    file->fd = open(file->file_path, O_RDONLY | O_LARGEFILE);
    if (file->fd == -1)
    {
        perror("ERROR: doesn't open sourse file");
        return ERROR_FILE_OPEN;
    }
    
    file->size   = size_file(file->fd);
    file->buffer = mmap(NULL, file->size, PROT_READ, MAP_SHARED, file->fd, 0);
    if (file->buffer == MAP_FAILED)
    {
        perror("ERROR: src mmap");

        close(file->fd);
        return ERROR_MMAP;
    }

    close(file->fd);
    return 0;
}

int sigconfigure(sigset_t* set)
{
    assert(set);

    sigemptyset(set);
    sigaddset(set, SIGUSR1);
    sigaddset(set, SIGUSR2);
    sigaddset(set, SIGTERM);
    
    struct sigaction act = {.sa_handler = handler_INT};
    sigaction(SIGINT, &act, NULL);

    if (sigprocmask(SIG_BLOCK, set, NULL) == -1)
    {
        perror("ERROR: sigprocmask");
        return  ERROR_SIGPROCMASK;
    }

    return 0;
}

void handler_TERM(int sig)
{
    if (snd_file.receiver_pid != snd_file.info.si_pid || 
        snd_file.info.si_code != SI_USER) return;

    if (munmap(snd_file.buffer, snd_file.size) == -1)
    {
        perror("ERROR: dst munmap");
    }

    exit(EXIT_FAILURE);
}

void handler_INT(int sig)
{
    kill(snd_file.receiver_pid, SIGINT);
    handler_TERM(sig);
}