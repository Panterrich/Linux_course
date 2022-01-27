#include <stdio.h>    
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/time.h>
#include <utime.h>
#include <limits.h>

enum ERRORS
{
    ERROR_INVALID_ARGC = -1,
    ERROR_OPEN         = -2,
    ERROR_FCNTL        = -3,
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Please run ./locker <arg>\n");
        return ERROR_INVALID_ARGC;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        perror("ERROR: open");
        return  ERROR_OPEN;
    }

    struct flock info = {.l_whence = SEEK_SET, .l_pid = -1};

    if (fcntl(fd, F_GETLK, &info) == -1)
    {
        perror("ERROR: fcntl");
        return  ERROR_FCNTL;
    }

    printf("\"%s\"\n"
           "info about filelock:\n"
           "l_type: ", argv[1]);

    if (info.l_type == F_WRLCK) printf("F_WRLCK\n");
    if (info.l_type == F_RDLCK) printf("F_RDLCK\n");
    if (info.l_type == F_UNLCK) printf("F_UNLCK\n");

    printf("l_whence: SEEK_SET\n"
           "l_start: %lu\n"
           "l_len: %lu\n"
           "l_pid: %d\n", info.l_start, info.l_len, info.l_pid);

    close(fd);

    return 0;
}