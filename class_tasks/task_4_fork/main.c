#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PROC 1023
#define MAX_LEN  100

enum ERRORS 
{
    ERROR_FORK           = 1,

    ERROR_DST_FILE_OPEN  = 2,
    ERROR_FEW_ARGC       = 3,
    ERROR_MANY_ARGC      = 4,
    ERROR_WRITE_DST_FILE = 5,
    ERROR_CLOSE_DST_FILE = 6,
    ERROR_WAITPID        = 7,
    ERROR_ZOMBIE         = 8
};

int main(int argc, char* argv[])
{
    if (argc < 2) 
    {
        printf("Please, enter 1 file path!\n");
        return ERROR_FEW_ARGC;
    }

    else if (argc > 2)
    {
        printf("Please, enter 1 file path!\n");
        return ERROR_MANY_ARGC;
    }

    char* dst_file_path  = argv[1];

    int fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return ERROR_DST_FILE_OPEN;
    }

    int   wstatus = 0;
    pid_t pid     = 0;

    for (int i = 0; i < MAX_PROC; ++i)
    {
        pid = fork();

        if (pid == -1)
        {
            perror("ERROR: fork");
            return  ERROR_FORK;
        }
        if (pid == 0) break;
    } 

    char buffer[MAX_LEN]  = "";
    int number_write_symb = snprintf(buffer, MAX_LEN, "PID: %d PPID: %d\n", getpid(), getppid());

    if (write(fd, buffer, number_write_symb) == -1)
    {
        perror("ERROR: doesn't write dst file");
        return  ERROR_WRITE_DST_FILE;
    }

    if (pid > 0)
    {
        pid_t pidchild = 0;

        while ((pidchild = waitpid(-1, &wstatus, 0)) > 0)
        {
            int code = WIFEXITED(wstatus);

            if (code)
            {
                code = WEXITSTATUS(wstatus);
                int number_write_symb = snprintf(buffer, MAX_LEN, "PID: %d EXIT_CODE: %d\n", pidchild, code);

                if (write(fd, buffer, number_write_symb) == -1)
                {
                    perror("ERROR: doesn't write dst file");
                    return  ERROR_WRITE_DST_FILE;
                }
            }

            else
            {
                perror("ERROR: zombie");
                return  ERROR_ZOMBIE;
            }
        }

        if (close(fd) == -1)
        {
            perror("ERROR: doesn't close dst file");
            return  ERROR_CLOSE_DST_FILE;
        }
    }

    return 0;
}