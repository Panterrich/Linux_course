#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

enum ERRORS
{
    ERROR_INVALID_ARGC = -1,
    ERROR_OPEN_DIR     = -2,
    ERROR_CLOSE_DIR    = -3,
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Please, enter 1 file paths!\n");
        return ERROR_INVALID_ARGC;
    }

    DIR* dir = opendir(argv[1]);

    if (!dir)
    {
        perror("ERROR: open dir");
        return  ERROR_OPEN_DIR;
    }

    struct dirent* ent = NULL;
    int count = 1;

    while (1)
    {
        ent = readdir(dir);
        if (ent == NULL) break;

        if ((ent->d_type & DT_DIR) == DT_DIR)
            printf("[%d] %s\n", count++, ent->d_name);
    } 

    if (closedir(dir) == -1)
    {
        perror("ERROR: close dir");
        return  ERROR_CLOSE_DIR;
    };
    return 0;
}