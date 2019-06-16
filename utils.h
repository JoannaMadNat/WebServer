#ifndef _UTILS_H
#define _UTILS_H
#include <stdio.h>    
#include <stdlib.h>     // Other standard library functions

#define BUFFSIZE 1024
#define MAXPATH  200

void blog(const char *fmt, ...);

typedef struct http_request
{
    char *verb;
    char *path;
    char *fullPath;
    char *version;
} http_request_t;

int pathCheck(char*);
int checkPermissions(http_request_t*);
void processExtension(char*, char**);
void mergeRootDirectory(char *,  char *, char**);
void parseHttp(FILE *, http_request_t **);
void revealTruePath(char*,char**);

//I WILL NOW REINVENT THE WHEEL!!!
int errjo;

#define HUZZAH   1 //on success
#define INHTTP  -1 // on invalid HTTP request
#define INVERB  -2 //on invalid HTTP request (Invalid verb)
#define INPATH  -3 //on invalid HTTP request (Invalid path)
#define INVERS  -4 //on invalid HTTP request (Invalid version)
#define INEOF   -5 //on invalid HTTP request (Premature EOF)
#define MALFAIL -6 //on allocation failure
#define IOFAIL  -7 //on I/O error
#define TOOLONG -8 //on string that is too long
#define NULLARG -9 //on NULL argument passed
#define ACCDEN  -10 //on file permission denied
#define NOFILE  -11 //on file does not exist
#define NOEXT   -12 //on extension not recognized
#define ISDIR   -13 //on link is directory
#define NOIMP   -14 //on no implementation
#define BLA     0   //on unexpected error

#endif