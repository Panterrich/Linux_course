#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>

//==============================================================================================

struct Command
{
    char** argv;
    size_t argc;
};

struct Text
{
    char* buffer;           
    struct Command* cmds; 
    size_t n_cmds;        
    size_t size;
};

//==============================================================================================

int text_ctor(int fd, struct Text* input_text);

size_t size_file(int fd);

char* create_buffer(int fd, struct Text* input_text);

size_t count_tokens(char* buffer, char sep);

struct Command* placing_pointers_in_text(char* buffer, const size_t n_cmds);

int get_argc_cmd(char* buffer);

char** get_argv_cmd(char* buffer, int argc);

void text_dtor(struct Text* text);

//============================================================================================

#endif // PARSER_H