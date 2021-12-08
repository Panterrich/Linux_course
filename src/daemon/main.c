#include "daemon.h"

int main()
{
    int ttl = 120;
    int delay = 5;
    if (daemonize(NULL, NULL, NULL, NULL, NULL) != 0)
    {
        perror("ERROR: daemonize");
        exit(EXIT_FAILURE);
    }

    while(ttl > 0) 
    {   
        syslog(LOG_NOTICE, "daemon ttl %d", ttl);
        sleep(delay);
        ttl -= delay;
    }

    syslog(LOG_NOTICE, "daemon ttl expired");
    closelog();

    return 0;
}
