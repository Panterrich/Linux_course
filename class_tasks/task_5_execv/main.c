#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_LEN 8192

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
    ERROR_READ_PIPE      = 10,
    ERROR_WRITE_FILE     = 11,
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
        char* dst_file_path = argv[1];

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
   
    int pipefd[2] = {};
    if (pipe2(pipefd, O_NONBLOCK) == -1)
    {
        perror("ERROR: doesn't create pipe");
        return  ERROR_CREATE_PIPE;
    }

    int   status = 0;
    pid_t pid = fork();

    if (pid == -1)
    {
       perror("ERROR: fork");
       return  ERROR_FORK;
    }

    if (pid == 0)
    {
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("ERROR: dup2");
            return  ERROR_DUP2;
        }

        if (execvp(argv[2], argv + 3))
        {
            perror("ERROR: execvp");
            return  ERROR_EXECVP;
        }
    }

    if (pid > 0)
    {
        if (wait(&status) == -1)
        {
            perror("ERROR: wait");
            return  ERROR_WAIT;
        }

        char* dst_file_path  = argv[1];

        int fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if (fd == -1)
        {
            perror("ERROR: doesn't open destination file");
            return  ERROR_DST_FILE_OPEN;
        }

        char buffer[MAX_LEN]  = "";
        int  number_read_symb = 0;
        
        do
        {
            number_read_symb = read(pipefd[0], buffer, MAX_LEN);

            if (number_read_symb == -1)
            {
                perror("ERROR: doesn't read pipe");
                return  ERROR_READ_PIPE;
            }

            if (number_read_symb == 0) break;

            if (write(fd, buffer, number_read_symb) == -1)
            {
                perror("ERROR: doesn't write dst file");
                return  ERROR_WRITE_FILE;
            }

        } while (1);

        if (close(fd) == -1)
        {
            perror("ERROR: doesn't close dst file");
            return  ERROR_CLOSE_DST_FILE;
        }

        return 0;
    }

    return 0;
}