#include <stdbool.h>    // For access to C99 "bool" type
#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Other standard library functions
#include <string.h>     // Standard string functions
#include <bsd/string.h> // Standard string functions
#include <errno.h>      // Global errno variable
#include <stdarg.h>     // Variadic argument lists (for blog function)
#include <time.h>       // Time/date formatting function (for blog function)
#include <unistd.h>     // Standard system calls
#include <signal.h>     // Signal handling system calls (sigaction(2))
#include <unistd.h>     //To check if file exists.

#include "eznet.h" // Custom networking library
#include "utils.h"

// Generic log-to-stdout logging routine
// Message format: "timestamp:pid:user-defined-message"
void blog(const char *fmt, ...)
{
    // Convert user format string and variadic args into a fixed string buffer
    char user_msg_buff[256];
    va_list vargs;
    va_start(vargs, fmt);
    vsnprintf(user_msg_buff, sizeof(user_msg_buff), fmt, vargs);
    va_end(vargs);

    // Get the current time as a string
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tp);

    // Print said string to STDOUT prefixed by our timestamp and pid indicators
    printf("%s:%d:%s\n", timestamp, getpid(), user_msg_buff);
}

//Checks if path is valid
//returns 1 if true
// returns 0 if false
int pathCheck(char *path)
{
    int ret = 1, i = 0;

    if (path[0] != '/' && path[0] != '.')
    {
        ret = 0;
        goto cleanup;
    }

    for (i = 1; i < strlen(path); i++)
        if (!((path[i] >= '0' && path[i] <= '9') || (path[i] >= 'a' && path[i] <= 'z') ||
              (path[i] >= 'A' && path[i] <= 'Z') || path[i] == '.' || path[i] == '/' ||
              path[i] == '_' || path[i] == '-'))
        {
            ret = 0;
            goto cleanup;
        }
cleanup:
    return ret;
}

//Takes in relative path and returns full path in result.
//returns status through errjo
void revealTruePath(char *path, char **result)
{
    errjo = BLA;
    char *real = NULL;

    if (path[0] != '.')
    {
        errjo = INPATH;
        goto cleanup;
    }

    real = realpath(path, NULL);
    if (!real)
    {
        switch (errno)
        {
        case EINVAL:
            errjo = NULLARG;
            break;
        case EACCES:
            errjo = ACCDEN;
            break;
        case EIO:
            errjo = IOFAIL;
            break;
        case ELOOP:
            errjo = INPATH;
            break;
        case ENOENT:
            errjo = NOFILE;
            break;
        case ENOTDIR:
            errjo = INPATH;
            break;
        case ENAMETOOLONG:
            errjo = TOOLONG;
            break;
        }
        goto cleanup;
    }
    errjo = HUZZAH;
    *result = real;
    real = NULL;

cleanup:
    free(real);
}

//Merges contents of rootdirectory entered throught the -r
//resulting string is dynamically allocated in resString
//returns status through errjo
void mergeRootDirectory(char *path, char *rootDirectory, char **resString)
{
    errjo = BLA;
    char *merge = NULL;

    if ((merge = calloc(sizeof(char), BUFFSIZE)) == NULL)
    {
        errjo = MALFAIL;
        goto cleanup;
    }

    if (!path || !rootDirectory)
    {
        errjo = NULLARG;
        goto cleanup;
    }

    if (strlcpy(merge, rootDirectory, BUFFSIZE) > BUFFSIZE || strlcat(merge, path, BUFFSIZE) > BUFFSIZE)
    {
        errjo = TOOLONG;
        goto cleanup;
    }

    *resString = merge;
    merge = NULL;
    errjo = HUZZAH;

cleanup:
    free(merge);
}

//Takes in http request and extracts parts of first line
//dynamically allocates them in request
//returns status through errjo
void parseHttp(FILE *in, http_request_t **request)
{
    http_request_t *req = NULL;
    char *line = NULL, *saveptr = NULL, *token = NULL, *headerLine = NULL;
    size_t lSize = 0;
    errjo = BLA;

    if ((req = calloc(sizeof(http_request_t), 100)) == NULL)
    {
        errjo = MALFAIL;
        goto cleanup;
    }
    if (getline(&line, &lSize, in) == -1)
    {
        errjo = IOFAIL;
        goto cleanup;
    }

    token = strtok_r(line, " ", &saveptr); //Extract verb
    if (!token)
    {
        errjo = IOFAIL;
        goto cleanup;
    }
    if (strcmp(token, "GET") != 0)
    {
        if (strcmp(token, "POST") == 0)
            errjo = NOIMP;
        else
            errjo = INVERB;
        goto cleanup;
    }
    req->verb = token;

    token = strtok_r(NULL, " ", &saveptr); //Extract path
    if (!token)
    {
        errjo = IOFAIL;
        goto cleanup;
    }
    int i = 0;
    for (i = 0; i <= MAXPATH; i++)
    {
        if (!token[i])
            break;
        
        if (i == MAXPATH && token[i] != '\0')
        {
            errjo = TOOLONG;
            goto cleanup;
        }
    }
    
    req->path = token;

    token = strtok_r(NULL, " ", &saveptr); //Extract version

    if (!token || !token[9] || !token[8])
    {
        errjo = IOFAIL;
        goto cleanup;
    }
    token[9] = '\0';
    token[8] = '\0';

    if (strcmp(token, "HTTP/1.0") != 0 && strcmp(token, "HTTP/1.1") != 0)
    {
        errjo = INVERS;
        goto cleanup;
    }
    req->version = token;

    errjo = IOFAIL;
    while (getline(&headerLine, &lSize, in) != -1 && !feof(in))
    {
        if (strcmp(headerLine, "\r\n") == 0 || (headerLine[0] == '\n' && headerLine[1] == '\0'))
        {
            errjo = HUZZAH;
            break;
        }
    }
    *request = req;
    req = NULL;
    line = NULL;

cleanup:
    free(line);
    free(req);
    free(headerLine);
}

//Checks link validity according to access
//returns errjo equivalent values
int checkPermissions(http_request_t *request)
{

    if (access(request->fullPath, F_OK) != -1)
        return HUZZAH;
    else if (access(request->path, F_OK) != -1)
        return ACCDEN;
    else
        return NOFILE;
}

#define EXTSIZE 20 //max size for file extensins and types
#define EXTNUM 19

//Determines file type and dynamically allocates it in type
//returns status in errjo
void processExtension(char *path, char **type)
{
    errjo = BLA;
    char *tp = NULL;
    int i;
    char extensions[EXTNUM][EXTSIZE] = {
        ".html", "text/html",
        ".htm", "text/html",
        ".css", "text/css",
        ".txt", "text/plain",
        ".js", "text/javascript",
        ".png", "image/png",
        ".gif", "image/gif",
        ".jpg", "image/jpg",
        ".jpeg", "image/jpeg",
        "Unknown"};

    char *dot = strrchr(path, '.');
    if (!dot)
    {
        errjo = ISDIR;
        goto cleanup;
    }

    tp = malloc(sizeof(char) * 20);
    if (!tp)
    {
        errjo = MALFAIL;
        goto cleanup;
    }

    for (i = 0; i < EXTNUM - 1; i += 2)
    {
        if (strcmp(dot, extensions[i]) == 0)
        {
            if (strlcpy(tp, extensions[i + 1], EXTSIZE) > EXTSIZE)
            {
                errjo = TOOLONG;
                goto cleanup;
            }
            *type = tp;
            errjo = HUZZAH;
            tp = NULL;
            goto cleanup; //Well that was quick...
        }
    }

    strlcpy(tp, extensions[EXTNUM - 1], EXTSIZE);
    *type = tp;
    errjo = HUZZAH;
    tp = NULL;

cleanup:
    free(tp);
}
