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
#include <sys/mman.h>
#include <string.h>

//==============================================================================

#define OR_DIE do {if (result) return result;} while(0)

//==============================================================================

enum ERRORS
{
    ERROR_INVALID_ARGC   = -1,
    ERROR_DST_FILE_OPEN  = -2,
    ERROR_TRUNCATE       = -3,
    ERROR_MMAP           = -4,
    ERROR_SIGPROCMASK    = -5,
};

struct Received_file
{
    pid_t sender_pid;
    uid_t sender_uid;
    siginfo_t info;

    int    fd;
    char*  file_path;

    char*  buffer;
    size_t size;
    size_t index;
};

//=================================================================================

size_t size_file(int fd);

int create_file(struct Received_file* file, char* file_name);

int sigconfigure(sigset_t* set);

void handler_USR1(int sig);
void handler_USR2(int sig);
void handler_TERM(int sig);

//==============================================================================

struct Received_file rcv_file = {};
int count = 0;
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Please, enter 1 src file path\n");
        return ERROR_INVALID_ARGC;
    }

    int result = create_file(&rcv_file, argv[1]); OR_DIE;

    sigset_t set = {};
    result = sigconfigure(&set); OR_DIE;
   
    pid_t pid_receiver = getpid();
    printf("PID for the transmission: %d\n", pid_receiver);

    int sig = 0;
    while (rcv_file.info.si_signo != SIGUSR1)
    {   
        sig = sigwaitinfo(&set, &rcv_file.info);
        
        switch (sig)
        {
            case -1:
                perror("ERROR: sigwaitinfo");
            case SIGTERM:
                handler_TERM(sig);
                break;
            case SIGUSR1:
                handler_USR1(sig);
                if (rcv_file.sender_pid == rcv_file.info.si_pid) kill(rcv_file.sender_pid, SIGUSR1);
                break;
            default:
                break;
        }
    }

    while (rcv_file.size != rcv_file.index)
    {   
        sig = sigwaitinfo(&set, &rcv_file.info);
        switch (sig)
        {
            case -1:
                perror("ERROR: sigwaitinfo");
            case SIGTERM:
                handler_TERM(sig);
                break;
            case SIGUSR2:
                handler_USR2(sig);
                kill(rcv_file.sender_pid, SIGUSR2);
                break;
            
            default:
                break;
        }
    }

    if (munmap(rcv_file.buffer, rcv_file.size) == -1)
    {
        perror("ERROR: dst munmap");
    }

    return 0;
}



//==============================================================================

size_t size_file(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}

int create_file(struct Received_file* file, char* file_name)
{
    assert(file);
    assert(file_name);

    file->file_path = file_name;
    file->fd        = open(file->file_path, O_RDWR | O_CREAT | O_LARGEFILE, 0666);

    if (rcv_file.fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return  ERROR_DST_FILE_OPEN;
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
    
    if (sigprocmask(SIG_BLOCK, set, NULL) == -1)
    {
        perror("ERROR: sigprocmask");
        return  ERROR_SIGPROCMASK;
    }

    return 0;
}

int resize_file(char* file_name, size_t size)
{
    assert(file_name);

    if (truncate(rcv_file.file_path, rcv_file.size) == -1)
    {
        perror("ERROR: truncate");
        return ERROR_TRUNCATE;
    }

    rcv_file.fd = open(rcv_file.file_path, O_RDWR | O_CREAT | O_LARGEFILE , 0666);
    if (rcv_file.fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return ERROR_DST_FILE_OPEN;
    }

    rcv_file.buffer = mmap(NULL, rcv_file.size, PROT_WRITE, MAP_SHARED, rcv_file.fd, 0);
    if (rcv_file.buffer == MAP_FAILED)
    {
        perror("ERROR: dst mmap");

        close(rcv_file.fd);
        return ERROR_MMAP;
    }

    close(rcv_file.fd);
    
    return 0;
}

//==============================================================================

void handler_USR1(int sig)
{
    if (rcv_file.sender_pid   != 0 || rcv_file.sender_uid != 0) return;
    if (rcv_file.info.si_code != SI_QUEUE) return;

    rcv_file.sender_pid = rcv_file.info.si_pid;
    rcv_file.sender_uid = rcv_file.info.si_uid;
    rcv_file.size       = rcv_file.info.si_int;

    int result = resize_file(rcv_file.file_path, rcv_file.size);
    if (result)
    {
        if (munmap(rcv_file.buffer, rcv_file.size) == -1)
        {
            perror("ERROR: dst munmap");
        }

        if (kill(rcv_file.sender_pid, SIGTERM) == -1)
        {
            perror("ERROR: kill to sender");
        }
        exit(EXIT_FAILURE);
    }
}

void handler_USR2(int sig)
{
    if (rcv_file.sender_pid != rcv_file.info.si_pid || 
        rcv_file.sender_uid != rcv_file.info.si_uid || 
                               rcv_file.info.si_code != SI_QUEUE) return;
  
    if (rcv_file.index + 7 < rcv_file.size) 
    {
        memcpy(rcv_file.buffer + rcv_file.index, &rcv_file.info.si_ptr, sizeof(void*));
        rcv_file.index += sizeof(void*);
        rcv_file.info.si_ptr = NULL;
    }
    else
    {
        memcpy(rcv_file.buffer + rcv_file.index, &rcv_file.info.si_ptr, rcv_file.size - rcv_file.index);
        rcv_file.index = rcv_file.size;
        
        if (munmap(rcv_file.buffer, rcv_file.size) == -1)
        {
            perror("ERROR: dst munmap");
        }
    }
}

void handler_TERM(int sig)
{
    if (rcv_file.sender_pid != rcv_file.info.si_pid || 
        rcv_file.sender_uid != rcv_file.info.si_uid || 
                               rcv_file.info.si_code != SI_QUEUE) return;

    if (munmap(rcv_file.buffer, rcv_file.size) == -1)
    {
        perror("ERROR: dst munmap");
    }

    exit(EXIT_FAILURE);
}