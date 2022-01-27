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
    if (argc != 4)
    {
        printf("Please run ./locker <arg1> <arg2> <arg3>\n");
        return ERROR_INVALID_ARGC;
    }

    size_t start = strtoul(argv[2], NULL, 10);
    size_t len   = strtoul(argv[3], NULL, 10);

    if (start == ULONG_MAX || len == ULONG_MAX)
    {
        printf("<arg2> and <arg3> must be integer");
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        perror("ERROR: open");
        return  ERROR_OPEN;
    }

    struct flock info = {.l_type = F_WRLCK, .l_start = start, .l_whence = SEEK_SET, .l_pid = -1, .l_len = len};

    if (fcntl(fd, F_SETLK, &info) == -1)
    {
        perror("ERROR: fcntl");
        return  ERROR_FCNTL;
    }

    printf("MY PID: %d\n", getpid());

    while (1) sleep(1);

    return 0;
}