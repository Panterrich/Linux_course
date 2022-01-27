#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

enum ERRORS 
{
    ERROR_LESS_ARGC      = 1,
    ERROR_MORE_ARGC      = 2,
    ERROR_FILE_OPEN      = 3,
    ERROR_CLOSE_SRC_FILE = 4,
    ERROR_SHMGET         = 5,
    ERROR_SHMAT          = 6,
    ERROR_READ_SRC_FILE  = 7,
    ERROR_SHMCTL         = 8,
    ERROR_SHMDT          = 9,
    ERROR_READ_PIPE      = 10,
    ERROR_WRITE_FILE     = 11,
    ERROR_MSGSND         = 12,
    ERROR_MSGCTL         = 13,
    ERROR_MSGGET         = 14,
};

#define SIZE_BUF 1000

struct Msg
{
    long mtype;
    size_t size;
    char buffer[SIZE_BUF];
};

size_t size_file(int fd);

int main(int argc, char* argv[])
{
    // if (argc < 2) 
    // {
    //     printf("Please, enter 1 src file path!\n");
    //     return ERROR_LESS_ARGC;
    // }

    // if (argc > 2)
    // {
    //     printf("Please, enter only 1 src file path!\n");
    //     return ERROR_MORE_ARGC;
    // }

    // char* src_file_path = argv[1];

    // int fd = open(src_file_path, O_RDONLY | O_LARGEFILE);

    // if (fd == -1)
    // {
    //     perror("ERROR: doesn't open sourse file");
    //     return ERROR_FILE_OPEN;
    // }
    
    // size_t fsize = size_file(fd);

    int msqid = msgget(0x1000 - 0x7, IPC_CREAT | 0666);

    if (msqid == -1)
    {
        perror("ERROR: msgget");
        return  ERROR_MSGGET;
    }

    struct Msg msg = {.mtype = 1};

    while (1)
    {
        if ((msg.size = read(STDIN_FILENO, msg.buffer, SIZE_BUF)) == -1)
        {
            perror("ERROR: doesn't read src file");
            return  ERROR_READ_SRC_FILE;
        }

        if (msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0) == -1)
        {
            perror("ERROR: msgsnd");
            return  ERROR_MSGSND;
        }

        // fsize -= msg.size;
    }   

    // msg.mtype = 0x1000 - 0x7;

    // if (msgsnd(msqid, &msg, sizeof(long), 0) == -1)
    // {
    //     perror("ERROR: msgsnd");
    //     return  ERROR_MSGSND;
    // }
    

    // if (close(fd) == -1)
    // {
    //     perror("ERROR: doesn't close src file");
    //     return  ERROR_CLOSE_SRC_FILE;
    // }

    getchar();
    
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("ERROR: msgctl");
        return  ERROR_MSGCTL;
    }

    return 0;
}

size_t size_file(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}