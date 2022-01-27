#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <limits.h>

enum ERRORS
{
    ISNOTDIR                =  1,
    
    ERROR_INVALID_ARGC      = -1,
    ERROR_OPEN_DIR          = -2,
    ERROR_CLOSE_DIR         = -3,
    ERROR_INOTIFY_INIT      = -4,
    ERROR_INOTIFY_ADD_WATCH = -5,
    ERROR_READ              = -6,
    ERROR_UNDEFINED_EVENT   = -7,
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Please, enter 1 dir path!\n");
        return ERROR_INVALID_ARGC;
    }

    int fd = inotify_init();
    if (fd == -1)
    {   
        perror("ERROR: idotify_init");
        return  ERROR_INOTIFY_INIT;
    }

    int result = inotify_add_watch(fd, argv[1], IN_ONLYDIR | IN_CREATE | IN_DELETE);
    if (result == ENOTDIR)
    {
        printf("Please, enter 1 dir path!\n");
        close(fd);
        return ISNOTDIR;
    }
    if (result == -1)
    {
        perror("ERROR: inotify_add_watch");
        close(fd);
        return ERROR_INOTIFY_ADD_WATCH;
    }

    int nsymb = 0;
    char buffer[sizeof(struct inotify_event) + NAME_MAX - 1] = {};

    while (1)
    {
        nsymb = read(fd, buffer, sizeof(struct inotify_event) + NAME_MAX - 1);

        if (nsymb == -1)
        {
            perror("ERROR: read");
            close(fd);
            return ERROR_READ;
        }

        if (nsymb > 0)
        {
            if (((struct inotify_event*)buffer)->mask & IN_CREATE)
            {
                printf("File \"%s\" was been created\n", ((struct inotify_event*)buffer)->name);
            }
            else if (((struct inotify_event*)buffer)->mask & IN_DELETE)
            {
                printf("File \"%s\" was been deleted\n", ((struct inotify_event*)buffer)->name);
            }
            else 
            {
                perror("ERROR: Undefined event");
                close(fd);
                return ERROR_UNDEFINED_EVENT;
            }
        }
    }

    return 0;
}