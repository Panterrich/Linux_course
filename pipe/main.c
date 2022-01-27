#define _GNU_SOURCE
#include "parser.h"
#include "pipe.h"

int main(int argc, char* argv[])
{
    if (argc != 2) 
    {
        printf("Please, enter 1 file paths!\n");
        return ERROR_ARGC;
    }

    char* src_file_path  = argv[1];

    int src_fd = open(src_file_path, O_RDONLY | O_LARGEFILE);

    if (src_fd == -1)
    {
        perror("ERROR: doesn't open sourse file");
        return ERROR_SRC_FILE_OPEN;
    }

    struct Text cmds = {};
    int res_ctor = text_ctor(src_fd, &cmds);

    if (close(src_fd) == -1)
    {
        perror("ERROR: doesn't close src file");
        text_dtor(&cmds);
        return ERROR_CLOSE_SRC_FILE;
    }

    if (res_ctor == 1) 
    {
        text_dtor(&cmds);
        return ERROR_ALLOCATE_MEMORY;
    }

    if (res_ctor == -1)
    {
        text_dtor(&cmds);
        return 0;
    }

    // printf("n_cdms: %d\n", cmds.n_cmds);
    // for (int i = 0; i < cmds.n_cmds; ++i)
    // {
    //     printf("cmd[%d] = \"", i);

    //     for (int j = 0; j < cmds.cmds[i].argc; ++j)
    //     {
    //         printf("\'%s\' ", cmds.cmds[i].argv[j]);
    //     }

    //     printf("\"\n");
    // }

    int res_exec = exec_commands(&cmds);

    if (!res_exec)
    {
        text_dtor(&cmds);
        return res_exec;
    }
    
    text_dtor(&cmds);
    return 0;
}