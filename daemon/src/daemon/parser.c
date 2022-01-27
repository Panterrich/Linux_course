#include "parser.h"
#include "daemon.h"
//==============================================================================================

int text_ctor(int fd, struct Text* input_text)
{
    assert(input_text);

    input_text->buffer = create_buffer(fd, input_text);
    if (!input_text->buffer) return 1;

    if (input_text->n_cmds == 0) return -1;

    input_text->cmds   = placing_pointers_in_text(input_text->buffer, input_text->n_cmds);
    if (!input_text->cmds)   return 1;

    return 0;
}

size_t size_file(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}

char* create_buffer(int fd, struct Text* input_text)
{
    assert(input_text);

    input_text->size = size_file(fd);

    char* buffer = (char*) calloc(input_text->size + 1, sizeof(char));

    if (!buffer)
    {
        syslog(LOG_ERR, "don't allocate memory");
        return buffer;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        syslog(LOG_ERR, "doesn't seek set src file");
        free(buffer);
        return NULL;
    }

    int number_read_symb = read(fd, buffer, input_text->size);

    if (number_read_symb == -1)
    {
        syslog(LOG_ERR, "doesn't read src file");
        free(buffer);
        return NULL;
    }

    if (number_read_symb != input_text->size)
    {
        syslog(LOG_ERR, "read");
        free(buffer);
        return NULL;
    }

    input_text->n_cmds = count_tokens(buffer, '|');

    return buffer;
}

size_t count_tokens(char* buffer, char sep)
{
    assert(buffer);

    char* begin = buffer;
    char* end   = buffer;

    size_t count = 0;

    while ((end = strchr(begin, sep)) != NULL)
    {
        begin = end + 1;
        ++count;
    }

    if (begin != buffer) ++count;

    return count;
}

struct Command* placing_pointers_in_text(char* buffer, const size_t n_cmds) 
{
    assert(buffer);
    assert(n_cmds > 0);

    struct Command* cmds = (struct Command*) calloc(n_cmds, sizeof(struct Command));

     if (!cmds)
    {
        syslog(LOG_ERR, "don't allocate memory");
        return cmds;
    }

    char* begin_cmd  = buffer;
    char* end        = buffer;

    int i = 0;

    for (; i < n_cmds - 1; ++i) 
    {
        end = strchr(begin_cmd, '|');
        *end = '\0';  

        while (isspace(*begin_cmd)) ++begin_cmd;

        cmds[i].argc = get_argc_cmd(begin_cmd);
        cmds[i].argv = get_argv_cmd(begin_cmd, cmds[i].argc);

        begin_cmd = end + 1;
    }

    while (isspace(*begin_cmd)) ++begin_cmd;

    cmds[i].argc = get_argc_cmd(begin_cmd);
    cmds[i].argv = get_argv_cmd(begin_cmd, cmds[i].argc);


    return cmds;
}

int get_argc_cmd(char* buffer)
{
    assert(buffer);

    char* begin = buffer;
    char* end   = buffer;

    size_t count = 1;

    while ((end = strchr(begin, ' ')) != NULL)
    {
        begin = end + 1;
        while (isspace(*begin)) ++begin;

        if (*begin == '\0') break;

        ++count;
    }

    return count;
}

char** get_argv_cmd(char* buffer, int argc)
{
    assert(buffer);

    char** argv = (char**) calloc(argc + 1, sizeof(char*));

    argv[argc] = NULL; // ТИЛЬТ!!!!!

    if (!argv) 
    {
        syslog(LOG_ERR, " don't allocate memory");
        return argv;
    }

    char* begin = buffer;
    char* end   = buffer;

    int i = 0;

    for (; i < argc - 1; ++i)
    {
        end = strchr(begin, ' ');
        if (end) *end = '\0';
        
        argv[i] = begin;

        begin = end + 1;
        while (isspace(*begin)) ++begin;

        if (*begin == '\0') break;
    }

    argv[i] = begin;

    end     = begin;
    while (*end != '\0' && !isspace(*end)) ++end;
    if (end) *end = '\0';

    return argv;
}

void text_dtor(struct Text* text)
{
    for (int i = 0; i < text->n_cmds; ++i)
    {
        free(text->cmds[i].argv);
        text->cmds[i].argv = NULL;
    }

    free(text->cmds);
    free(text->buffer);
    
    text->cmds   = NULL;
    text->buffer = NULL;
}