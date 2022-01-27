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
    ERROR_DST_FILE_OPEN  = 3,
    ERROR_CLOSE_DST_FILE = 4,
    ERROR_SHMGET         = 5,
    ERROR_SHMAT          = 6,
    ERROR_READ_SRC_FILE  = 7,
    ERROR_SHMCTL         = 8,
    ERROR_SHMDT          = 9,
    ERROR_READ_PIPE      = 10,
    ERROR_WRITE_FILE     = 11,
    ERROR_MSGGET         = 12,
    ERROR_MSGCTL         = 13,
};

#define SIZE_BUF 1000

struct Msg
{
    long mtype;
    size_t size;
    char buffer[SIZE_BUF];
};

int main(int argc, char* argv[])
{
    int msqid = msgget(0x1000 - 0x7, 0);

    if (msqid == -1)
    {
        perror("ERROR: msgget");
        return  ERROR_MSGGET;
    }
    
    struct Msg msg = {};
    
    while (msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), 0, 0) != -1)
    {
        if (msg.mtype == 0x1000 - 0x7) break;

        if (msg.mtype == 1)
        {
            if (write(STDOUT_FILENO, msg.buffer, msg.size) == -1)
            {
                perror("ERROR: doesn't write dst file");
                return  ERROR_WRITE_FILE;
            }
        }
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("ERROR: msgctl");
        return  ERROR_MSGCTL;
    }
}