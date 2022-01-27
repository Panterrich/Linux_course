#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

//========================================================

#define MAX_LEN 8192

enum ERRORS
{
    ERROR_FEW_ARGC                            =  1,
    ERROR_MANY_ARGC                           =  2,

    ERROR_SRC_FILE_OPEN                       = -3,
    ERROR_DST_FILE_OPEN                       = -4,

    ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS = -5,

    ERROR_READ_SRC_FILE                       = -6,
    ERROR_WRITE_DST_FILE                      = -7,

    ERROR_CLOSE_SRC_FILE                      = -8,
    ERROR_CLOSE_DST_FILE                      = -9
};

//========================================================


size_t file_size(int fd);

int copy_file(char* src_file_path, char* dst_file_path);

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
        return 0;
    }

    return copy_file(src_file_path, dst_file_path);
}

int copy_file(char* src_file_path, char* dst_file_path)
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

    size_t size          = file_size(src_fd); 
    char buffer[MAX_LEN] = "";

    while (size > 0)
    {
        int number_read_symb = read(src_fd, buffer, MAX_LEN);

        if (number_read_symb == -1)
        {
            perror("ERROR: doesn't read src file");
            return ERROR_READ_SRC_FILE;
        }

        int number_write_symb = write(dst_fd, buffer, number_read_symb);

        if (number_write_symb == -1)
        {
            perror("ERROR: doesn't write dst file");
            return ERROR_WRITE_DST_FILE;
        }

        if (number_write_symb != number_read_symb)
        {
            errno = ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS;
            perror("ERROR: number of read symbols doesn't match number of write symbols");
            return ERROR_NOT_MATCH_NUMBER_READ_WRITE_SYMBOLS;
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

    return 0;
}

size_t file_size(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}