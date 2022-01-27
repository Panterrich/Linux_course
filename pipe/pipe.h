#ifndef PIPE_H
#define PIPE_H

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "parser.h"

enum ERRORS
{
    ERROR_ARGC                                =  -1,

    ERROR_SRC_FILE_OPEN                       = -3,

    ERROR_NULLPTR_WORD                        = -5,
    ERROR_SEEK_FILE                           = -6,
    ERROR_UNDEFINED_TYPE_WORD                 = -7,
    ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS = -8,

    ERROR_READ_SRC_FILE                       = -9,

    ERROR_CLOSE_SRC_FILE                      = -11,
    ERROR_CLOSE_DST_FILE                      = -12,

    ERROR_WRITE_WORD_INCORRECTLY              = -13,
    ERROR_ALLOCATE_MEMORY                     = -14,
    ERROR_PIPE                                = -15,
    ERROR_WAIT                                = -16,
    ERROR_DUP2                                = -17,
    ERROR_EXECVP                              = -18
};

int exec_commands(struct Text* cmds);

#endif // PIPE_H