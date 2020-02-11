#include <stdbool.h>    // For access to C99 "bool" type
#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Other standard library functions
#include <string.h>     // Standard string functions
#include <bsd/string.h> // Standard string functions
#include <errno.h>      // Global errno variable

#include <stdarg.h> // Variadic argument lists (for blog function)
#include <time.h>   // Time/date formatting function (for blog function)

#include <unistd.h> // Standard system calls
#include <signal.h> // Signal handling system calls (sigaction(2))

#include <sys/types.h> //Includes for wait3
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

// signal handler for SIGCHILD
// Cleans up all children who have died
void killChildren(int signum)
{
    while (wait3((int *)NULL, WNOHANG, (struct rusage *)NULL) > 0)
        ;
}

int main()
{
    int attacks = 1000;
    char *port[] = {"./ddos.sh", "5000", NULL};
    int i;
    printf("doing stuff...");

    for (i = 0; i < attacks; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            execv("./ddos.sh", port);
            perror("exec");
            goto cleanup;
        }
        if (pid < 0)
        {
            printf("poop...\n");
        }
        else
        {
            //Install signal handler for SIGCHLD
            struct sigaction sa_child = {
                .sa_handler = killChildren};
            if (sigaction(SIGCHLD, &sa_child, NULL))
            {
                goto cleanup;
            }
            
        }
    }
cleanup:
    return 0;
}