/* Echo Server: an example usage of EzNet
 * (c) 2016, Bob Jones University
 */
#include <stdbool.h>    // For access to C99 "bool" type
#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Other standard library functions
#include <string.h>     // Standard string functions
#include <bsd/string.h> // Standard string functions
#include <errno.h>      // Global errno variable
#include <sys/stat.h>     //To use mkdir
#include <sys/types.h>

#include <stdarg.h> // Variadic argument lists (for blog function)
#include <time.h>   // Time/date formatting function (for blog function)

#include <unistd.h> // Standard system calls
#include <signal.h> // Signal handling system calls (sigaction(2))

#include <sys/types.h> //Includes for wait3
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "eznet.h"        // Custom networking library
#include "httpResponse.h" // Custom http responses
#include "utils.h"

// GLOBAL: settings structure instance
struct settings
{
    const char *bindhost; // Hostname/IP address to bind/listen on
    const char *bindport; // Portnumber (as a string) to bind/listen on
    char *rootdir;        //Root directory for path
    long maxChilds;
} g_settings = {
    .bindhost = "localhost", // Default: listen only on localhost interface
    .bindport = "5000",      // Default: listen on TCP port 5000
    .rootdir = "./templates", // Default: root directory is /templates
    .maxChilds = 5,
};

// Parse commandline options and sets g_settings accordingly.
// Returns 0 on success, -1 on false...
int parse_options(int argc, char *const argv[])
{
    int ret = -1;
    long testnum;
    char *endptr = NULL;
    char op;
    while ((op = getopt(argc, argv, "h:p:r:w:")) > -1)
    {
        switch (op)
        {
        case 'h':
            g_settings.bindhost = optarg;
            break;
        case 'p':
            g_settings.bindport = optarg;

            testnum = strtol(optarg, &endptr, 10);
            if (errno == ERANGE || (errno != 0 && testnum == 0) || *endptr != '\0' || endptr == optarg)
                goto cleanup;

            break;
        case 'r':
            g_settings.rootdir = optarg;
            if (!pathCheck(g_settings.rootdir))
                goto cleanup;

            int result = mkdir(g_settings.rootdir, 0777);
            if(result!= 0 && errno != EEXIST)
                goto cleanup;

            break;
        case 'w':
            g_settings.maxChilds = strtol(optarg, &endptr, 10);
            if (errno == ERANGE || (errno != 0 && g_settings.maxChilds == 0) || *endptr != '\0' || endptr == optarg)
                goto cleanup;

            break;
        default:
            // Unexpected argument--abort parsing
            goto cleanup;
        }
    }

    ret = 0;
cleanup:
    return ret;
}

// GLOBAL: flag indicating when to shut down server
volatile bool server_running = false;
int activeChildren = 0;

// SIGINT handler that detects Ctrl-C and sets the "stop serving" flag
void sigint_handler(int signum)
{
    blog("Ctrl-C (SIGINT) detected; shutting down...");
    server_running = false;
}

// SIGINT handler that detects Ctrl-C and sets the "stop serving" flag
void signals_handler(int signum)
{
    blog("Connection interrupted: re-estalishing connection...");
}

// signal handler for SIGCHILD
// Cleans up all children who have died
void waitchildren(int signum)
{
    while (wait3((int *)NULL,
                 WNOHANG,
                 (struct rusage *)NULL) > 0)
    {
        --activeChildren;
        blog("A client disconnected.\n\tActive clients: %d", activeChildren);
    }
}

//sends bad http response bassed on errjo value
//path is optional, pass NULL if no path
void analyzeErrjo(FILE *stream, char *path)
{
    switch (errjo) //Behold! The wall of possible errjors.
    {
    case HUZZAH:
        break;
    case INHTTP:
        badHTTP(stream, "400", "Illegal request", "Illegal HTTP stream.", path);
        break;
    case IOFAIL:
        badHTTP(stream, "500", "I/O error", "I/O error while reading request.", path);
        break;
    case INVERB:
        badHTTP(stream, "400", "Illegal request", "Illegal HTTP stream (Invalid verb).", path);
        break;
    case INPATH:
        badHTTP(stream, "400", "Illegal request", "Illegal HTTP stream (Invalid path).", path);
        break;
    case INVERS:
        badHTTP(stream, "400", "Illegal request", "Illegal HTTP stream (Invalid version).", path);
        break;
    case INEOF:
        badHTTP(stream, "400", "Illegal request", "Illegal HTTP stream (Premature EOF).", path);
        break;
    case TOOLONG:
        badHTTP(stream, "400", "Illegal request", "Request contains a string that is too long.", path);
        break;
    case NOIMP:
        badHTTP(stream, "501", "Not implemented", "This verb has not been implemented.", path);
        break;
    case NOFILE:
        badHTTP(stream, "404", "Not Found", "This file does not exist.", path);
        break;
    case ACCDEN:
        badHTTP(stream, "403", "Forbidden", "You do not have permission to access this file.", path);
        break;
    case NULLARG:
        badHTTP(stream, "400", "NULL Argument", "You have given me a NULL argument you uncultured swine.", path);
        break;
    case NOEXT:
        badHTTP(stream, "400", "Invalid file", "Cannot open file because extention is not recognized.", path);
        break;
    case ISDIR:
        badHTTP(stream, "403", "Forbidden", "You do not have permission to open directories.\r\nShame on you.", path);
        break;
    default:
        badHTTP(stream, "400", "Unexpected error", "Hold at T-31 due to an unexpected error.", path);
    }
}

// Connection handling logic: reads/echos lines of text until error/EOF,
// then tears down connection.
void handle_client(struct client_info *client)
{
    FILE *stream = NULL;

    // Wrap the socket file descriptor in a read/write FILE stream
    // so we can use tasty stdio functions like getline(3)
    // [dup(2) the file descriptor so that we don't double-close;
    // fclose(3) will close the underlying file descriptor,
    // and so will destroy_client()]
    if ((stream = fdopen(dup(client->fd), "a+")) == NULL) //changed this to a+... can it beee??? IT'S FIXED!!!! MY NIGHTMARE IS OVERRRRRR!!!!!!!!!!!!!!!!
    {
        perror("Unable to wrap socket");
        goto cleanup;
    }

    http_request_t *request = NULL;
    char *fileType = NULL;

    parseHttp(stream, &request);
    if (errjo != 1)
    {
        analyzeErrjo(stream, NULL);
        goto cleanup;
    }

    processExtension(request->path, &fileType);
    if (errjo != 1)
    {
        analyzeErrjo(stream, request->path);
        //I'm going insane right now.
        goto cleanup;
    }

    if (request->path[0] == '.')
    {
        revealTruePath(request->path, &request->fullPath);
        if (errjo != 1 || !request->fullPath)
        {
            analyzeErrjo(stream, request->path);
            //90% of the time spent on this code is spent on making it look pretty.
            goto cleanup;
        }
        //Now I can reject it
        errjo = ACCDEN;
        analyzeErrjo(stream, request->fullPath);
        goto cleanup;
    }

    mergeRootDirectory(request->path, g_settings.rootdir, &request->fullPath);
            printf("%s\n", request->fullPath);

    if (errjo != 1 || !request->fullPath)
    {
        analyzeErrjo(stream, request->path);
        goto cleanup;
    }

    if ((errjo = checkPermissions(request)) != 1)
    {
        analyzeErrjo(stream, request->path);
        goto cleanup;
    }

    httpOK(stream, request->fullPath, fileType);

cleanup:
    // Shutdown this client
    free(fileType);

    if (request != NULL)
    {
        free(request->verb);
        free(request->fullPath);
    }
    free(request);
    if (stream)
        fclose(stream);
    destroy_client_info(client);
    printf("\tSession ended.\n");
}

//recieve client's request and reject it,
//Better that ignoring client's request and leaving their windiow blocked forever
void rejectClient(struct client_info *client)
{
    FILE *stream = NULL;
    if ((stream = fdopen(dup(client->fd), "a")) == NULL)
    //with 'a' <- server says: "I don't care what you have to say, you will listen to me right now."
    {
        perror("Unable to wrap socket");
        goto cleanup;
    }

    fputs("Server busy, connection failed...\n", stream);

cleanup:
    if (stream)
        fclose(stream);
    destroy_client_info(client);
}

int installHandlers()
{
    int i = 0;
    // Install signal handler for SIGINT
    struct sigaction sa_int = {
        .sa_handler = sigint_handler};
    if (sigaction(SIGINT, &sa_int, NULL) || sigaction(SIGQUIT, &sa_int, NULL))
    {
        LOG_ERROR("sigaction(SIGINT, ...) -> '%s'", strerror(errno));
        return -1;
    }

    // Install signal handler for SIGINT
    struct sigaction sigs_handler = {
        .sa_handler = signals_handler,
        .sa_flags = SA_RESTART};
    for (i = 1; i <= 31; i++)
    {
        if (i == 2 || i == 15 || i == 9 || i == 3 || i == 19 || i == 17) //skip SIGINT, SIGTERM, SIGKILL, SIGQUIT, SIGSTOP, SIGCHLD
            continue;
        if (sigaction(i, &sigs_handler, NULL))
        {
            LOG_ERROR("sigaction(%d, ...) -> '%s'", i, strerror(errno));
            return -1;
        }
    }

    return 1;
}

int main(int argc, char **argv)
{
    int ret = 1, pid = 0;
    // Network server/client context
    int server_sock = -1;

    // Handle our options
    if (parse_options(argc, argv))
    {
        printf("usage: %s [-p PORT] [-h HOSTNAME/IP] [-r /ROOT/PATH] [-w MaxClients]\n", argv[0]);
        goto cleanup;
    }

    if (installHandlers() == -1)
    {
        goto cleanup;
    }

    if (strcmp(g_settings.rootdir, "") == 0)
    {
        g_settings.rootdir = getcwd(NULL, BUFFSIZE);
        if (!g_settings.rootdir)
        {
            LOG_ERROR("getcwd -> '%s'", strerror(errno));
            goto cleanup;
        }
    }

    // Start listening on a given port number
    server_sock = create_tcp_server(g_settings.bindhost, g_settings.bindport);
    if (server_sock < 0)
    {
        perror("unable to create socket");
        goto cleanup;
    }
    blog("\nBound and listening on %s:%s\nRoot directory set at %s\nMaximum clients set to %d", g_settings.bindhost, g_settings.bindport, g_settings.rootdir, g_settings.maxChilds);

    server_running = true;
    while (server_running)
    {
        //NOT TO SELF
        //Master socket is taken from  create_tcp_server()   and is stored in server_sock
        //worker socket is taken from    wait_for_client()   and is stored in client->fd

        struct client_info client;

        // Wait for a connection on that socket
        if (wait_for_client(server_sock, &client))
        {
            // Check to make sure our "failure" wasn't due to
            // a signal interrupting our accept(2) call; if
            // it was  "real" error, report it, but keep serving.
            if (errno != EINTR)
                perror("Unable to accept connection");
        }
        else
        {
            if (activeChildren < g_settings.maxChilds)
            {
                ++activeChildren;
                blog("Connection from %s:%d\n\tActive clients: %d", client.ip, client.port, activeChildren);
                pid = fork();
                if (pid < 0)
                {
                    perror("fork");
                }
                else if (pid == 0)
                { //CHILD PROCESS (client handler)
                    if (server_sock)
                        close(server_sock);
                    handle_client(&client); // Client gets cleaned up in here
                                                goto cleanup;
                                            }
                        else
                        { //PARENT PROCESS (server)
                            if (client.fd)
                                close(client.fd);

                            //Install signal handler for SIGCHLD
                            struct sigaction sa_child = {
                                .sa_handler = waitchildren};
                            if (sigaction(SIGCHLD, &sa_child, NULL))
                            {
                                LOG_ERROR("sigaction(SIGCHLD, ...) -> '%s'", strerror(errno));
                                goto cleanup;
                            }
                        }
                    }
                    else
                    {
                        rejectClient(&client);
                    }
        }
    }
    ret = 0;
    pid++;
cleanup:
    if (server_sock >= 0)
        close(server_sock);
    return ret;
}
