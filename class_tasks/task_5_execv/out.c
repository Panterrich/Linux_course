#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

enum ERRORS 
{
    ERROR_DST_FILE_OPEN  = 2,
    ERROR_FEW_ARGC       = 3,
    ERROR_CLOSE_DST_FILE = 4,
    ERROR_CREATE_PIPE    = 5,
    ERROR_FORK           = 6,
    ERROR_EXECVP         = 7,
    ERROR_DUP2           = 8,
    ERROR_WAIT           = 9,
};

int main(int argc, char* argv[])
{
    if (argc < 2) 
    {
        printf("Please, enter 1 file path!\n");
        return ERROR_FEW_ARGC;
    }

    else if (argc == 2)
    {
        char* dst_file_path  = argv[1];

        int fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if (fd == -1)
        {
            perror("ERROR: doesn't open destination file");
            return  ERROR_DST_FILE_OPEN;
        }

        if (close(fd) == -1)
        {
            perror("ERROR: doesn't close dst file");
            return  ERROR_CLOSE_DST_FILE;
        }

        return 0;
    }

    char* dst_file_path  = argv[1];

    int fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return  ERROR_DST_FILE_OPEN;
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("ERROR: dup2");
        return  ERROR_DUP2;
    }

    if (execvp(argv[2], argv + 2))
    {
        perror("ERROR: execvp");
        return  ERROR_EXECVP;
    }


    return 0;
}