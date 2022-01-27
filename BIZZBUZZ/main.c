#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

//========================================================

#define MAX_LEN 4096

enum ERRORS
{
    ERROR_FEW_ARGC                            =  1,
    ERROR_MANY_ARGC                           =  2,
    ERROR_SAME_NAMES_FILES                    =  3,

    ERROR_SRC_FILE_OPEN                       = -3,
    ERROR_DST_FILE_OPEN                       = -4,

    ERROR_NULLPTR_WORD                        = -10,
    ERROR_SEEK_FILE                           = -11,
    ERROR_UNDEFINED_TYPE_WORD                 = -12,
    ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS = -5,

    ERROR_READ_SRC_FILE                       = -6,
    ERROR_WRITE_DST_FILE                      = -7,

    ERROR_CLOSE_SRC_FILE                      = -8,
    ERROR_CLOSE_DST_FILE                      = -9,

    ERROR_WRITE_WORD_INCORRECTLY              = -13
};

enum BIZZBUZZ
{
    DEFAULT  = 0,
    STR      = 1,
    BIZZ     = 2,
    BUZZ     = 3, 
    BIZZBUZZ = 4,

    CONTINUE = 5,
    NEXT     = 6,
    SPACE    = 7
};

//========================================================

size_t file_size(int fd);

int find_word(char* buffer, size_t size, char** word, size_t* word_size);

int print_word(int dst_fd, int type, int src_fd, size_t offset_file, size_t size);

int anylise_word(char* word, size_t len, int mod);

int print_bizzbuzz(char* src_file_path, char* dst_file_path);

//========================================================

int main(int argc, char* argv[])
{
    if (argc < 3) 
    {
        printf("Please, enter 2 file paths!\n");
        return ERROR_FEW_ARGC;
    }

    else if (argc > 3)
    {
        printf("Please, enter 2 file paths!\n");
        return ERROR_MANY_ARGC;
    }

    char* src_file_path  = argv[1];
    char* dst_file_path  = argv[2];

    if (strcmp(src_file_path, dst_file_path) == 0)
    {
        printf("Please, enter different 2 file paths!\n");
        return ERROR_SAME_NAMES_FILES;
    }

    return print_bizzbuzz(src_file_path, dst_file_path);
}

int print_bizzbuzz(char* src_file_path, char* dst_file_path)
{
    assert(src_file_path);
    assert(dst_file_path);

    int src_fd = open(src_file_path, O_RDONLY | O_LARGEFILE);

    if (src_fd == -1)
    {
        perror("ERROR: doesn't open sourse file");
        return ERROR_SRC_FILE_OPEN;
    }

    int dst_fd = open(dst_file_path, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, 0666);

    if (dst_fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return ERROR_DST_FILE_OPEN;
    }

    size_t size            = file_size(src_fd); 

    char   buffer[MAX_LEN] = "";
    int    mode            = NEXT;
    
    char*  word            = NULL;
    size_t size_word       = 0;
    size_t long_word_size  = 0;
    int    type            = DEFAULT;

    size_t offset_file     = 0;
    
    size_t offset_read     = 0;
    
    while (size > 0)
    {
        size_t offset_buf = 0;

        int number_read_symb = read(src_fd, buffer, MAX_LEN);
        offset_read += number_read_symb;
        
        if (number_read_symb == -1)
        {
            perror("ERROR: doesn't read src file");
            return ERROR_READ_SRC_FILE;
        }

        if (mode == CONTINUE && number_read_symb > 0 && isspace(buffer[offset_buf]))
        {
            int error = print_word(dst_fd, type, src_fd, offset_file, long_word_size);
            if (error) return error;

            if (lseek64(src_fd, offset_read, SEEK_SET) == -1)
            {
                errno = ERROR_SEEK_FILE;
                perror("ERROR: doesn't seek set src file");
                return  ERROR_SEEK_FILE;
            }

            offset_file   += long_word_size;
            long_word_size = 0;
            type           = DEFAULT;
            mode           = NEXT;
        } 

        while (offset_buf != number_read_symb)
        {   
            mode = find_word(buffer + offset_buf, number_read_symb - offset_buf, &word, &size_word);
            offset_buf += size_word;
           
            if      (mode == SPACE)  type = STR;
            else if (mode == NEXT || type != STR)
            {
                type = anylise_word(word, size_word, mode);
            }
            
            if (type != STR && type != BIZZ && type != BUZZ && type != BIZZBUZZ)
            {   
                errno = ERROR_UNDEFINED_TYPE_WORD;
                perror("ERROR: undefined word type was obtained");
                return  ERROR_UNDEFINED_TYPE_WORD;
            }

            if (mode == NEXT || mode == SPACE || size == offset_buf)
            {    
                int error = print_word(dst_fd, type, src_fd, offset_file, long_word_size + size_word);
                if (error) return error;

                if (lseek64(src_fd, offset_read, SEEK_SET) == -1)
                {
                    errno = ERROR_SEEK_FILE;
                    perror("ERROR: doesn't seek set src file");
                    return  ERROR_SEEK_FILE;
                }

                offset_file += long_word_size + size_word;
                type         = DEFAULT;
            }
            
            if (mode == CONTINUE) long_word_size += size_word;
            if (mode == NEXT)     long_word_size = 0;
        }

        size -= number_read_symb; 
    }


    if (close(src_fd) == -1)
    {
        perror("ERROR: doesn't close src file");
        return ERROR_CLOSE_SRC_FILE;
    }

    if (close(dst_fd) == -1)
    {
        perror("ERROR: doesn't close dst file");
        return ERROR_CLOSE_DST_FILE;
    }
}

int find_word(char* buffer, size_t size, char** word, size_t* word_size)
{
    assert(buffer);
    assert(word);
    assert(word_size);

    char*  begin_word   = buffer;
    size_t length       = 0;   

    while (isspace(begin_word[length]) && size > 0) 
    {   
        --size;
        ++length;
    }

    if (length != 0)
    {
        *word      = begin_word;
        *word_size = length;
        return SPACE;
    }


    while (size > 0)
    {   
        if (isspace(begin_word[length]))
        {
            *word      = begin_word;
            *word_size = length;

            return NEXT;
        }

        ++length;
        --size;
    }

    *word      = begin_word;
    *word_size = length;
   
    return CONTINUE;
}



int anylise_word(char* word, size_t len, int mode)
{
    if (!word) return ERROR_NULLPTR_WORD;

    static int num_mod_3  = 0;
    static int last_digit = 1;

    if (mode == NEXT)
    {
        num_mod_3  = 0;
        last_digit = 1;
    }

    size_t offset = 0;

    if (len == 0)                 return STR;
    if (*word == '-' && len == 1) return STR;

    if (*word == '-') 
    {
        ++offset;
        ++word;
    }

    while (isdigit(*word) && (offset < len))
    {
        last_digit = *word - '0';
        num_mod_3  = (last_digit + num_mod_3) % 3;

        ++offset;
        ++word;
    }

         if (offset    != len)                      return STR;
    else if (num_mod_3 == 0 && last_digit % 5 == 0) return BIZZBUZZ;
    else if (num_mod_3 == 0)                        return BIZZ;
    else if                   (last_digit % 5 == 0) return BUZZ;
    else                                            return STR;
}



int print_word(int dst_fd, int type, int src_fd, size_t offset_file, size_t size_word)
{
    static char buffer[MAX_LEN] = "";
    static char bizz[]          = "bizz";
    static char buzz[]          = "buzz";
    static char bizzbuzz[]      = "bizzbuzz";

    switch (type)
    {
        case BIZZ:
        {
            if (write(dst_fd, bizz, 4) == -1)
            {
                perror("ERROR: doesn't write dst file");
                return  ERROR_WRITE_DST_FILE;
            }
            return 0;
        }

        case BUZZ:
        {
            if (write(dst_fd, buzz, 4) == -1)
            {
                perror("ERROR: doesn't write dst file");
                return  ERROR_WRITE_DST_FILE;
            }
            return 0;
        }

        case BIZZBUZZ:
        {
            if (write(dst_fd, bizzbuzz, 8) == -1)
            {
                perror("ERROR: doesn't write dst file");
                return  ERROR_WRITE_DST_FILE;
            }
            return 0;
        }

        case STR:
        {
            if (lseek64(src_fd, offset_file, SEEK_SET) == -1)
            {
                perror("ERROR: doesn't seek set src file");
                return  ERROR_SEEK_FILE;
            }

            int number_read_symb = 0;
            
            while (size_word > 0)
            {
                if (size_word > MAX_LEN) number_read_symb = read(src_fd, buffer, MAX_LEN);
                else                     number_read_symb = read(src_fd, buffer, size_word);
                
                if (number_read_symb == -1)
                {
                    perror("ERROR: doesn't read src file");
                    return  ERROR_READ_SRC_FILE;
                }

                int number_write_symb = write(dst_fd, buffer, number_read_symb);

                if (number_write_symb == -1)
                {
                    perror("ERROR: doesn't write dst file");
                    return  ERROR_WRITE_DST_FILE;
                }

                if (number_write_symb != number_read_symb)
                {
                    errno = ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS;
                    perror("ERROR: number of read symbols doesn't match number of write symbols");
                    return  ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS;
                }

                size_word -= number_read_symb;
            }
            
            if (size_word == 0) return 0;
            else
            {
                errno = ERROR_WRITE_WORD_INCORRECTLY;
                perror("ERROR: more or fewer characters were written than needed");
                return  ERROR_WRITE_WORD_INCORRECTLY;
            }
        }
    
        default:
        {
            errno = ERROR_UNDEFINED_TYPE_WORD;
            perror("ERROR: undefined word type");
            return  ERROR_UNDEFINED_TYPE_WORD;
        }
    }
}

size_t file_size(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}