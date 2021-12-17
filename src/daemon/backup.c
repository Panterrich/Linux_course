#include "daemon.h"

#define MAX_LEN 4096

struct hashtable table = {};

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

    int dst_fd = open(dst_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

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

void get_new_ver_dir(char* version_directory, char* dst_directory, char* sep)
{
    time_t now = time(NULL);
    struct tm tstruct = *localtime(&now);
    char date[1001] = "";
    
    strftime(date, sizeof(date), "%d.%m.%Y-%X", &tstruct);
    sprintf(version_directory, "%s/%s-%s", dst_directory, sep, date);
}

int check_include_directory(char* src_directory, char* dst_directory)
{   
    char *first = strstr(dst_directory, src_directory);
    char *second = strstr(src_directory, dst_directory);

    if (first  && *(first  + strlen(src_directory)) == '/') return 1;
    if (second && *(second + strlen(dst_directory)) == '/') return 1;

    return 0;
}

void backup_inotify(char* src_directory, char* dst_directory, int inotify_config)
{
    if (inotify_config) hashtable_clear(&table);

    if (!check_include_directory(src_directory, dst_directory))
    {
        char dst_current_version[3003] = "";
        get_new_ver_dir(dst_current_version, dst_directory, "scr-inotify");

        syslog(LOG_NOTICE, "inotify backup started in \"%s\"", dst_current_version);

        if (!check_dst(dst_directory))
        {
            syslog(LOG_ERR, "dst check error");
            shutdown(0);
        };

        if (!check_dst(dst_current_version))
        {
            syslog(LOG_ERR, "dst check error");
            shutdown(0);
        };

        struct stat src_info = {};
        if (stat(src_directory, &src_info) == -1)
        {
            syslog(LOG_ERR, "stat src directory error");
            shutdown(0);
        }

        if (chmod(dst_current_version, src_info.st_mode) == -1)
        {
            syslog(LOG_ERR, "chmod directory \"%s\" error", dst_current_version);
            shutdown(0);
        }

        int result = 0;

        if (inotify_config)
        {
            syslog(LOG_NOTICE, "inotify backup full");
            result = backup_inotify_full(src_directory, dst_current_version);
            sleep(2);
        }
        else
        {   
            syslog(LOG_NOTICE, "inotify backup inc");
            result = backup_inotify_inc(src_directory, dst_current_version);
            sleep(2);
        }

        if (result == 1)
        {
            syslog(LOG_NOTICE, "inotify backup not completed");
        }
            
        else if (result == 0)
        {
            syslog(LOG_NOTICE, "inotify backup completed");
        }
            
        else if (result == -1)
        {
            syslog(LOG_NOTICE, "there are no changes in inotify backup");
            rmdir(dst_current_version);
        }
    }
    else
    {
        syslog(LOG_NOTICE, "inotify backup was ignored, please, change source or destination directory");
    }
}

int backup_inotify_full(char* src_path, char* dst_path)
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

        int fd_inotify = inotify_init1(O_NONBLOCK);
        if (fd_inotify == -1)
        {
            syslog(LOG_ERR, "inotify_init1");
            closedir(src_dir);
            return 1;
        }

        int wd_inotify = inotify_add_watch(fd_inotify, src_path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_EXCL_UNLINK | \
                                                                 IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
        if (wd_inotify == -1)
        {
            syslog(LOG_ERR, "inotify_add_watch");
            closedir(src_dir);
            close(fd_inotify);
            return 1;
        } 

        hashtable_set_value(&table, src_path, fd_inotify);

        struct dirent* src_entry = NULL;

        while ((src_entry = readdir(src_dir)) != NULL)
        {
            if (strcmp(src_entry->d_name, ".")  == 0) continue;
            if (strcmp(src_entry->d_name, "..") == 0) continue;

            snprintf(src_name, MAX_LEN, "%s/%s", src_path, src_entry->d_name);
            snprintf(dst_name, MAX_LEN, "%s/%s", dst_path, src_entry->d_name);

            syslog(LOG_NOTICE, "inotify backup \"%s\"", src_name);

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

                    if (backup_inotify_full(src_name, dst_name)) 
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
        syslog(LOG_ERR, "backup_inotify_full(src_path) when src_path \"%s\" isn't directory", src_name);
    }

    return 0;
}

int backup_inotify_inc(char* src_directory, char* dst_directory)
{
    sleep(10);

    char buffer[MAX_LEN] = "";

    char src_name[MAX_LEN] = "";
    char dst_name[MAX_LEN] = "";

    struct stat src_info = {};

    int new_modification = -1;

    for (int i = 0; i < table.size; ++i)
    {
        struct list_pointer_t* current_element = table.keys[i].head;

        while (current_element != NULL)
        {   
            syslog(LOG_NOTICE, "Check event %s", current_element->key);

            memset(buffer,   0, MAX_LEN);
            memset(src_name, 0, MAX_LEN);
            memset(dst_name, 0, MAX_LEN);

            int result = read(current_element->value, buffer, MAX_LEN - 1);

            if (result == -1 && errno == EAGAIN)
            {
                current_element = current_element->next;
                errno = 0;
                continue;
            }

            syslog(LOG_NOTICE, "New event %s", current_element->key);
            new_modification = 0;
            
            if (((struct inotify_event*)buffer)->mask && IN_CREATE)
            {
                snprintf(src_name, MAX_LEN, "%s/%s", current_element->key, ((struct inotify_event*)buffer)->name);
                syslog(LOG_NOTICE, "%s backup", src_name);

                char* res = strstr(src_name, src_directory);
                if (res == NULL)
                {
                    syslog(LOG_ERR, "%s src file is not in \"%s\"", src_name, src_directory);
                    return 1;
                }

                res = src_name + strlen(src_directory) + 1;
                snprintf(dst_name, MAX_LEN, "%s/%s", dst_directory, res);

                if (create_directories(dst_name, dst_directory, src_name, src_directory))
                {
                    return 1;
                }

                if (lstat(src_name, &src_info))
                {
                    syslog(LOG_ERR, "stat \"%s\" file", src_name);
                    return 1;
                }

                if (S_ISDIR(src_info.st_mode))
                {
                    if (mkdir(dst_name, src_info.st_mode) == -1)
                    {
                        syslog(LOG_ERR, "mkdir \"%s\" error", dst_name);
                        return 1;
                    }

                    if (backup_inotify_full(src_name, dst_name)) 
                    {   
                        return 1;
                    }
                }

                if (S_ISREG(src_info.st_mode))
                {
                    if (copy_file(src_name, dst_name))
                    {   
                        syslog(LOG_ERR, "copy file \"%s\" was not done", src_name);
                        return 1;
                    }

                    if (chmod(dst_name, src_info.st_mode) == -1)
                    {
                        syslog(LOG_ERR, "chmod \"%s\" was not done", dst_name);
                        return 1;
                    }
                }

                if (S_ISFIFO(src_info.st_mode))
                {
                    if (mkfifo(dst_name, src_info.st_mode) == -1)
                    {
                        syslog(LOG_ERR, "mkfifo \"%s\" error", src_name);
                        return 1;
                    }
                }

                if (S_ISLNK(src_info.st_mode))
                {
                    if (copy_link(src_name, dst_name))
                    {
                        syslog(LOG_ERR, "copy link \"%s\" was not done", src_name);
                        return 1;
                    }
                }
            }
        }
    }

    return new_modification;
}

int create_directories(char* dst_name, char* dst_directory, char* src_name, char* src_directory)
{
    char* dst_new_dir = NULL;
    char* src_cur_dir = NULL;

    char name_src_dir[MAX_LEN] = "";
    char name_dst_dir[MAX_LEN] = "";

    struct stat info = {};

    dst_new_dir = strchr(dst_name + strlen(dst_directory) + 1, '/');
    src_cur_dir = strchr(src_name + strlen(src_directory) + 1, '/');

    while (dst_new_dir != NULL)
    {
        strncpy(name_dst_dir, dst_name, dst_new_dir - dst_name);
        strncpy(name_src_dir, src_name, src_cur_dir - src_name);

        if (stat(name_src_dir, &info)) return 1;

        mkdir(name_dst_dir, info.st_mode);

        dst_new_dir = strchr(dst_new_dir + 1, '/');
        src_cur_dir = strchr(src_cur_dir + 1, '/');
    }

    return 0;
}