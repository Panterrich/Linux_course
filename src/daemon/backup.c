#include "daemon.h"

#define MAX_LEN 4096

int check_args(char* src, char* dst)
{
    DIR* dir = opendir(src);

    if (!dir)
    {   
        printf("Source directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    return check_dst(dst);
}


int check_dst(char* dst)
{
    DIR* dir = opendir(dst);
                    
    if (!dir && errno == ENOENT)
    {
        if (mkdir(dst, 0666) == -1)
        {
            perror("ERROR: mkdir");
            return 0;
        }
    }
    else if (!dir)
    {
        printf("Destination directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    return 1;
}

int backup(char* src_path, char* dst_path)
{
    char src_name[MAX_LEN] = "";
    char dst_name[MAX_LEN] = "";

    struct stat src_info = {};

    int result = stat(src_path, &src_info);
    if (result == -1)
    {
        syslog(LOG_ERR, "stat src file");
        return 1;
    }

    if (S_ISDIR(src_info.st_mode))
    {
        DIR* src_dir = opendir(src_path);
        if (!src_dir)
        {
            syslog(LOG_ERR, "opendir src directory");
            return 1;
        }

        struct dirent* src_entry = NULL;

        while ((src_entry = readdir(src_dir)) != NULL)
        {
            if (strcmp(src_entry->d_name, ".")  == 0) continue;
            if (strcmp(src_entry->d_name, "..") == 0) continue;

            snprintf(src_name, MAX_LEN, "%s/%s", src_path, src_entry->d_name);
            snprintf(dst_name, MAX_LEN, "%s/%s", dst_path, src_entry->d_name);

            syslog(LOG_NOTICE, "backup \"%s\"", src_name);

            switch (src_entry->d_type)
            {
                case DT_FIFO:
                {
                    struct stat info_current_file = {};
                    if (stat(src_name, &info_current_file) == -1)
                    {   
                        closedir(src_dir);
                        syslog(LOG_ERR, "stat \"%s\" file", src_name);
                        return 1;
                    }

                    if (mkfifo(dst_name, info_current_file.st_mode))
                    {
                        closedir(src_dir);
                        syslog(LOG_ERR, "mkfifo \"%s\" error", src_name);
                        return 1;
                    }
                }
                break;

                case DT_REG:
                {
                    struct stat info_current_file = {};
                    if (stat(src_name, &info_current_file) == -1)
                    {   
                        closedir(src_dir);
                        syslog(LOG_ERR, "stat \"%s\" file", src_name);
                        return 1;
                    }

                    if (copy_file(src_name, dst_name))
                    {   
                        closedir(src_dir);
                        syslog(LOG_ERR, "copy file \"%s\" was not done", src_name);
                        return 1;
                    }

                    if (chmod(dst_name, info_current_file.st_mode) == -1)
                    {
                        closedir(src_dir);
                        syslog(LOG_ERR, "chmod \"%s\" was not done", dst_name);
                        return 1;
                    }
                }
                break;

                case DT_LNK:
                {
                    if (copy_link(src_name, dst_name))
                    {
                        closedir(src_dir);
                        syslog(LOG_ERR, "copy link \"%s\" was not done", src_name);
                        return 1;
                    }
                }
                break;

                case DT_DIR:
                {
                    struct stat info_current_file = {};
                    if (stat(src_name, &info_current_file) == -1)
                    {   
                        closedir(src_dir);
                        syslog(LOG_ERR, "stat \"%s\" file", src_name);
                        return 1;
                    }

                    if (mkdir(dst_name, info_current_file.st_mode) == -1)
                    {
                        closedir(src_dir);
                        syslog(LOG_ERR, "mkdir \"%s\" error", dst_name);
                        return 1;
                    }

                    if (backup(src_name, dst_name)) 
                    {   
                        closedir(src_dir);
                        return 1;
                    }
                }
                break;
            
                default:
                {
                    syslog(LOG_NOTICE, "\"%s\" was ignored", src_name);
                }
                break;
            }
        }

        memset(src_name, 0, MAX_LEN);
        memset(dst_name, 0, MAX_LEN);

        closedir(src_dir);
    }
    else
    {
        syslog(LOG_ERR, "backup(src_path) when src_path \"%s\" isn't directory", src_name);
    }

    return 0;
}

int search(char* dir_path, char* file)
{
    DIR* dir = opendir(dir_path);
    if (!dir)
    {
        syslog(LOG_ERR, "opendir src directory");
        return 0;
    }

    struct dirent* src_entry = NULL;

    while ((src_entry = readdir(dir)) != NULL)
    {
        if (strcmp(src_entry->d_name, file) == 0)
        {
            closedir(dir);
            return 1;
        }       
    }

    closedir(dir);
    return 0;
}

int copy_file(char* src_file_path, char* dst_file_path)
{
    if (strcmp(src_file_path, dst_file_path) == 0)
    {
        return 0;
    }

    int src_fd = open(src_file_path, O_RDONLY | O_LARGEFILE);

    if (src_fd == -1)
    {
        syslog(LOG_ERR, "doesn't open sourse file");
        return 1;
    }

    int dst_fd = open(dst_file_path, O_WRONLY | O_CREAT| O_TRUNC, 0666);

    if (dst_fd == -1)
    {
        syslog(LOG_ERR, "doesn't open destination file \"%s\"", dst_file_path);
        return 1;
    }

    size_t size          = file_size(src_fd); 
    char buffer[MAX_LEN] = "";

    while (size > 0)
    {
        int number_read_symb = read(src_fd, buffer, MAX_LEN);

        if (number_read_symb == -1)
        {
            syslog(LOG_ERR, "doesn't read src file");
            return 1;
        }

        int number_write_symb = write(dst_fd, buffer, number_read_symb);

        if (number_write_symb == -1)
        {
            syslog(LOG_ERR, "doesn't write dst file");
            return 1;
        }

        if (number_write_symb != number_read_symb)
        {
            syslog(LOG_ERR, "number of read symbols doesn't match number of write symbols");
            return 1;
        }

        size -= number_read_symb; 
    }

    close(src_fd);
    close(dst_fd);

    return 0;
}

int copy_link(char* src_file_path, char* dst_file_path)
{
    if (strcmp(src_file_path, dst_file_path) == 0)
    {
        return 0;
    }
    
    struct stat info = {};
    if (lstat(src_file_path, &info) == -1)
    {
        syslog(LOG_ERR, "lstat error");
        return 1;
    }

    char* buffer = (char*) calloc(info.st_size + 1, sizeof(char));

    if (readlink(src_file_path, buffer, info.st_size) != info.st_size) 
    {   
        free(buffer);
        syslog(LOG_ERR, "readlink error");
        return 1;
    }
   
    buffer[info.st_size] = '\0';

    if (symlink(buffer, dst_file_path))
    {   
        free(buffer);
        syslog(LOG_ERR, "symblink error");
        return 1;
    }
    
    free(buffer);
    return 0;
}

size_t file_size(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}