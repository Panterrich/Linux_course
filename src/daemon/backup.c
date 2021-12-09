#include "daemon.h"

int check_args(char* src, char* dst)
{
    DIR* dir = opendir(src);

    if (!dir)
    {   
        printf("Source directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    dir = opendir(dst);
                    
    if (!dir && errno == ENOENT)
    {
        if (mkdir(dst, 0666))
        {
            perror("ERROR: mkdir");
            return 0;
        }
    }
    else if (!dir)
    {
        printf("Destination directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    return 1;
}