#include "pipe.h"
#include "parser.h"

int exec_commands(struct Text* cmds)
{
    assert(cmds);

    int* pipes = (int*) calloc (cmds->n_cmds - 1, 2 * sizeof(int));

    if (!pipes)
    {
        perror("ERROR: don't allocate memory for pipes");
        return  ERROR_ALLOCATE_MEMORY;
    }

    for (int i = 0; i < cmds->n_cmds - 1; ++i)
    {
        if (pipe(pipes + 2 * i) == -1)
        {
            perror("ERROR: pipe()");
            return  ERROR_PIPE;
        }
    }

    int status = 0;
    pid_t pid  = 0;

    for (int i = 0; i < cmds->n_cmds; ++i)
    {
        if ((pid = fork()) == 0) // child
        {   
            if (i != 0)
            {
                if (dup2(pipes[2 * (i - 1)], STDIN_FILENO) == -1)
                {
                    perror("ERROR: dup2()");
                    return  ERROR_DUP2;
                }
            }

            if (i != cmds->n_cmds - 1)
            {
                if (dup2(pipes[2 * i + 1], STDOUT_FILENO) == -1)
                {
                    perror("ERROR: dup2()");
                    return  ERROR_DUP2;
                }
            }

            for (int j = 0; j < 2 * (cmds->n_cmds - 1); ++j)
            {
                if (close(pipes[j]) == -1)
                {
                    perror("ERROR: close pipes");
                    return  ERROR_CLOSE_DST_FILE;
                }
            }

            if (execvp(cmds->cmds[i].argv[0], cmds->cmds[i].argv) == -1)
            {
                fprintf(stderr, "ERROR: EXECVP: ");

                for (int j = 0; j < cmds->cmds[i].argc; ++j)
                {
                    fprintf(stderr, "%s ", cmds->cmds[i].argv[j]);
                }
                
                fprintf(stderr, "\n");

                return ERROR_EXECVP;
            }
        }
    }

    for (int i = 0; i < 2 * (cmds->n_cmds - 1); ++i)
    {
        if (close(pipes[i]) == -1)
        {
            perror("ERROR: close pipes");
            return  ERROR_CLOSE_DST_FILE;
        }
    }

    for (int i = 0; i < cmds->n_cmds; ++i)
    {
        pid = wait(&status);

        if (pid == -1)
        {
            perror("ERROR: wait()");
            return  ERROR_WAIT;
        }
    }
    
    free(pipes);

    return 0;
}