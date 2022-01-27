#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
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
};

int main(int argc, char* argv[])
{
    if (argc < 2) 
    {
        printf("Please, enter 1 src file path!\n");
        return ERROR_LESS_ARGC;
    }

    if (argc > 2)
    {
        printf("Please, enter only 1 src file path!\n");
        return ERROR_MORE_ARGC;
    }

    char* dst_file_path = argv[1];

    int fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return  ERROR_DST_FILE_OPEN;
    }

    int shmid = shmget(0x1000 - 0x7, 0, 0);

    if (shmid == -1)
    {
        perror("ERROR: shmget");
        return  ERROR_SHMGET;
    }
    
    struct shmid_ds info = {};
    if (shmctl(shmid, IPC_STAT, &info) == -1)
    {
        perror("ERROR: shmctl");
        return  ERROR_SHMCTL;
    }

    char* buffer = (char*) shmat(shmid, NULL, 0);

    if (buffer == (char*) -1)
    {
        perror("ERROR: shmat");
        return  ERROR_SHMAT;
    }

    if (write(fd, buffer, info.shm_segsz - 1) == -1)
    {
        perror("ERROR: doesn't write dst file");
        return  ERROR_WRITE_FILE;
    }

    if (close(fd) == -1)
    {
        perror("ERROR: doesn't close dst file");
        return  ERROR_CLOSE_DST_FILE;
    }

    if (shmdt(buffer) == -1)
    {
        perror("ERROR: shmdt");
        return  ERROR_SHMDT;
    }
}