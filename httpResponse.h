#include <stdio.h>    
#include <stdlib.h>     // Other standard library functions

#ifndef _HTTPRESPONSES_H
#define _HTTPRESPONSES_H

void httpOK(FILE*, char*, char*);
void badHTTP(FILE*, char* , char* , char*, char*);
int startsWith(const char *, const char *);

#endif