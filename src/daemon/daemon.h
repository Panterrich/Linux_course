#ifndef DAEMON_H
#define DAEMON_H

#define _LARGEFILE64_SOURCE

#include <stdio.h>    
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/time.h>
#include <utime.h>
#include <wait.h>
#include <limits.h>
#include <time.h>

#define MAX_LEN 4096


//=========================================================================================

enum MODE
{
    MODE_CLASSIC = 0,
    MODE_INOTIFY = 1,
};

//=========================================================================================

int check_args(char* src, char* dst);

int check_dst(char* dst);

int daemonize(char* name, char* path, char* in_file, char* out_file, char* err_file);

void sighandler_configuration();

void sigmask_configuration(sigset_t* wait);

char* create_path(char* prefix, char* name, char* postfix);

void create_pid_file(char* name);

void create_and_lock_pid_file(char* name);

void lock_pid_file(int fd_pid, char* name);

void create_and_open_fifo(char* name);

void close_all_fd();

void open_in_out_err(char* in_file, char* out_file, char* err_file);

void shutdown(int num);

int dump(char *dst_path);

//=========================================================================================

int search(char* dir_path, char* file);

int backup(char* src_path, char* dst_path);

int copy_file(char* src_file_path, char* dst_file_path);

int copy_link(char* src_file_path, char* dst_file_path);

size_t file_size(int fd);

#endif // DAEMON_H