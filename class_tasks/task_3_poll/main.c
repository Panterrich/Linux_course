#include <stdio.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAXARGS 8192
#define MAXLEN  1000


enum ERRORS 
{
    ERROR_MANY_ARGS  = 1,

    ERROR_OPEN_FILE  = 2,
    ERROR_POLL       = 3,
    ERROR_READ_FILE  = 4,
    ERROR_CLOSE_FILE = 5,
};

int main(int argc, char* argv[]) 
{
    char buffer[MAXLEN + 1] = "";


    if (argc == 1)          return 0;
    if (argc - 1 > MAXARGS) return ERROR_MANY_ARGS; 

    int number_files = argc - 1;

    struct pollfd plfd[MAXARGS] = {};

    for (int i = 0; i < number_files; ++i)
    {
        plfd[i].fd = open(argv[i + 1], O_RDWR | O_NONBLOCK);

        if (plfd[i].fd == -1)
        {
            sprintf(buffer, "ERROR: unable to open file \"%s\"", argv[i + 1]);
            perror(buffer);
            return ERROR_OPEN_FILE;
        }

        plfd[i].events = POLLIN;
    }

    while (poll(plfd, number_files, -1) != -1) 
    {
        for (int i = 0; i < number_files; i++)
        {   
            if (plfd[i].revents == POLLERR)
            {
                perror("ERROR: poll error");
                return ERROR_POLL;
            }

            if (plfd[i].revents == POLLIN)
            {   
                plfd[i].events  = 0;
                plfd[i].revents = 0;

                int number_read_symb = 1;

                while (number_read_symb != 0)
                {
                    number_read_symb = read(plfd[i].fd, buffer, MAXLEN);

                    if (number_read_symb == -1)
                    {
                        sprintf(buffer, "ERROR: unable to read file \"%s\"", argv[i + 1]);
                        perror(buffer);
                        return ERROR_READ_FILE;
                    }

                    buffer[number_read_symb] = '\0';

                    fprintf(stdout, "%s", buffer);
                    fflush(stdout);
                }
            }

            if (plfd[i].revents == POLLHUP)
            {
                if (close(plfd[i].fd) == -1)
                {
                    sprintf(buffer, "ERROR: unable to close file \"%s\"", argv[i + 1]);
                    perror(buffer);
                    return ERROR_CLOSE_FILE;
                }

                plfd[i].events = 0;
            } 
        }
    }

    perror("ERROR: poll error");
    return ERROR_POLL;
}
