#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

enum ERRORS 
{
    ERROR_SIGUSR1 = -1,
    ERROR_SIGUSR2 = -2,
};

void USR1_HANDLER(int signum);
void USR2_HANDLER(int signum);

int main()
{
    struct sigaction usr1_new = {}, usr1_old = {}, usr2_new = {}, usr2_old = {};

    usr1_new.sa_handler = USR1_HANDLER;
    usr2_new.sa_handler = USR2_HANDLER;


    printf("My PID: %d\n\n", getpid());

    int error = sigaction(SIGUSR1, &usr1_new, &usr1_old);

    if (error == -1)
    {
        perror("ERROR: sigaction(SIGUSR1, ....)");
        return  ERROR_SIGUSR1;
   
    }

    error = sigaction(SIGUSR2, &usr2_new, &usr2_old);

    if (error == -1)
    {
        perror("ERROR: sigaction(SIGUSR2, ....)");
        return  ERROR_SIGUSR2;
    }
    
    while (1) sleep(1);

    return 0;
}

void USR1_HANDLER(int signum)
{
    printf("London ");
    fflush(stdout);
}

void USR2_HANDLER(int signum)
{
    printf("is the capital of Great Britain\n");
}