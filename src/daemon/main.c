#include "daemon.h"

extern int fd_fifo;

int main(int argc, char *argv[])
{
    if (argc != 1 && argc != 3)
    {
        printf("Please run daemon without args or specify 2 directories\n");
        return 1;
    }

    char src_directory[1001] = "";
    char dst_directory[1001] = "";

    if (argc == 3)
    {
        if (!check_args(argv[1], argv[2]))
        {
            return 1;
        }

        strncpy(src_directory, argv[1], 1000);
        strncpy(dst_directory, argv[2], 1000);
    }

    if (daemonize(NULL, NULL, NULL, NULL, NULL) != 0)
    {
        perror("ERROR: daemonize");
        exit(EXIT_FAILURE);
    }

    static int delay = 60;
    static int mode = 0;

    sigset_t sig = {};
    sigmask_configuration(&sig);

    alarm(delay);
    int signum = 0;
    siginfo_t info = {};

    int run = 1;

    while (1)
    {
        signum = sigwaitinfo(&sig, &info);

        switch (signum)
        {
        case SIGALRM:
        {
            alarm(delay);

            int result = 0;
            char *first = strstr(dst_directory, src_directory);
            char *second = strstr(src_directory, dst_directory);

            if (first)
            {
                if (*(first + strlen(src_directory)) == '/')
                    result = 1;
            }

            if (second)
            {
                if (*(second + strlen(dst_directory)) == '/')
                    result = 1;
            }

            if (!result)
            {
                time_t now = time(NULL);
                struct tm tstruct = *localtime(&now);
                char date[1001] = "";
                char dst_current_version[3003] = "";
                strftime(date, sizeof(date), "%d.%m.%Y-%X", &tstruct);
                sprintf(dst_current_version, "%s/src-%s", dst_directory, date);

                syslog(LOG_NOTICE, "backup started in \"%s\"", dst_current_version);

                if (!check_dst(dst_directory))
                {
                    syslog(LOG_ERR, "dst check error");
                    shutdown(signum);
                };

                if (!check_dst(dst_current_version))
                {
                    syslog(LOG_ERR, "dst check error");
                    shutdown(signum);
                };

                struct stat src_info = {};
                if (stat(src_directory, &src_info) == -1)
                {
                    syslog(LOG_ERR, "stat src directory error");
                    shutdown(signum);
                }

                if (chmod(dst_current_version, src_info.st_mode) == -1)
                {
                    syslog(LOG_ERR, "chmod directory \"%s\" error", dst_current_version);
                    shutdown(signum);
                }

                if (backup(src_directory, dst_current_version))
                {
                    syslog(LOG_NOTICE, "backup not completed");
                }
                else
                {
                    syslog(LOG_NOTICE, "backup completed");
                }
            }
            else
            {
                syslog(LOG_NOTICE, "backup was ignored, please, change source or destination directory");
            }
        }
        break;

        case SIGUSR1:
        {
            if (info.si_int == -1)
            {
                info.si_int = 0;
                syslog(LOG_NOTICE, "controller check this demon");
                break;
            }

            delay = (info.si_int) > 0 ? info.si_int : delay;
            syslog(LOG_NOTICE, "controller set a new interval: %d", delay);
            info.si_int = 0;
        }
        break;

        case SIGUSR2:
        {
            if (info.si_int == 0)
            {
                memset(src_directory, 0, 1001);

                if (read(fd_fifo, src_directory, 1000) == -1)
                {
                    syslog(LOG_ERR, "daemon can't read src directory");
                    break;
                }
                if (info.si_int == -2)
                {
                    info.si_int = 0;
                    if (dump(dst_directory))
                    {
                        syslog(LOG_NOTICE, "dump wasn't completed");
                    }
                    break;
                }

                DIR *dir = opendir(src_directory);

                if (!dir)
                {
                    syslog(LOG_WARNING, "controller send novalid directory path");
                    break;
                }

                closedir(dir);

                if (strcmp(src_directory, dst_directory))
                {
                    syslog(LOG_WARNING, "source and destination directory are the same, so backup will be ignored");
                }

                syslog(LOG_NOTICE, "controller set a new src directory: \"%s\"", src_directory);
            }
            else
            {
                memset(dst_directory, 0, 1001);

                if (read(fd_fifo, dst_directory, 1000) == -1)
                {
                    syslog(LOG_ERR, "daemon can't read src directory");
                    break;
                }

                DIR *dir = opendir(dst_directory);

                if (!dir && errno == ENOENT)
                {
                    if (mkdir(dst_directory, 0666))
                    {
                        syslog(LOG_ERR, "daemon can't make directory ");
                        break;
                    }
                }
                else if (!dir)
                {
                    syslog(LOG_WARNING, "controller send novalid directory path");
                    break;
                }

                closedir(dir);

                if (strcmp(src_directory, dst_directory))
                {
                    syslog(LOG_WARNING, "source and destination directory are the same, so backup will be ignored");
                }

                syslog(LOG_NOTICE, "controller set a new dst directory: \"%s\"", dst_directory);
            }
        }
        break;

        case SIGHUP:
        {
            mode = info.si_int;

            if (mode == MODE_CLASSIC)
            {
                syslog(LOG_NOTICE, "controller set CLASSIC mode");
            }
            else if (mode == MODE_INOTIFY)
            {
                syslog(LOG_NOTICE, "controller set INOTIFY mode");
            }
            else
            {
                mode = MODE_CLASSIC;
                syslog(LOG_NOTICE, "controller set UNDEFINED mode, so set default mode - CLASSIC");
            }

            info.si_int = 0;
        }
        break;

        case SIGQUIT:
        case SIGTERM:
        {
            syslog(LOG_NOTICE, "terminated");
            shutdown(signum);
        }
        break;

        case SIGINT:
        {
            if (run)
            {
                sigdelset(&sig, SIGALRM);
                syslog(LOG_NOTICE, "daemon POWER[%d] has been stopped", getpid());
            }
            else
            {
                sigaddset(&sig, SIGALRM);
                syslog(LOG_NOTICE, "daemon POWER[%d] has been started", getpid());
            }

            run = !run;
        }
        break;

        default:
            syslog(LOG_WARNING, "undefined signal");
            break;
        }
    }

    syslog(LOG_NOTICE, "finished");
    closelog();

    return 0;
}
