#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

//========================================================

#define MAX_LEN 8192

enum ERRORS
{
    ERROR_FEW_ARGC                            = -1,
    ERROR_MANY_ARGC                           = -2,

    ERROR_SRC_FILE_OPEN                       = -3,
    ERROR_DST_FILE_OPEN                       = -4,

    ERROR_MMAP                                = -5,
    ERROR_MUNMAP                              = -6,
    ERROR_TRUNCATE                            = -7
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

    size_t size   = file_size(src_fd);
    char* src_buf = mmap(NULL, size, PROT_READ, MAP_SHARED, src_fd, 0);
    
    if (src_buf == MAP_FAILED)
    {
        perror("ERROR: src mmap");

        close(src_fd);
        return ERROR_MMAP;
    }

    int dst_fd = open(dst_file_path, O_RDWR | O_CREAT | O_LARGEFILE, 0666);

    if (dst_fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return ERROR_DST_FILE_OPEN;
    }

    close(dst_fd);

    if (truncate(dst_file_path, size) == -1)
    {
        perror("ERROR: truncate");

        close(src_fd);
        close(dst_fd);
        if (munmap(src_buf, size) == -1)
        {
            perror("ERROR: src munmap");
        }

        return ERROR_TRUNCATE;
    }

    dst_fd = open(dst_file_path, O_RDWR | O_CREAT | O_LARGEFILE , 0666);

    if (dst_fd == -1)
    {
        perror("ERROR: doesn't open destination file");
        return ERROR_DST_FILE_OPEN;
    }

    char* dst_buf = mmap(NULL, size, PROT_WRITE, MAP_SHARED, dst_fd, 0);
    
    if (dst_buf == MAP_FAILED)
    {
        perror("ERROR: dst mmap");

        close(src_fd);
        close(dst_fd);
        if (munmap(src_buf, size) == -1)
        {
            perror("ERROR: src munmap");
        }
        return ERROR_MMAP;
    }
    
    memcpy(dst_buf, src_buf, size);

    if (munmap(src_buf, size) == -1)
    {
        perror("ERROR: src munmap");
    }

    if (munmap(dst_buf, size) == -1)
    {
        perror("ERROR: dst munmap");
    }

    close(src_fd);
    close(dst_fd);
    return 0;
}

size_t file_size(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}