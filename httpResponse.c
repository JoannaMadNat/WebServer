#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Other standard library functions
#include <bsd/string.h> // Standard string functions
#include "utils.h"
#include "httpResponse.h"

//HTTP response when requests passes all tests and file can be given
void httpOK(FILE *stream, char *path, char *contentType)
{
    fflush(stream);

    FILE *page = NULL;
    size_t size;
    char *line = NULL;

    page = fopen(path, "r");
    if (!page) //This is very unlikely because there is already an earlier check.
    {
        badHTTP(stream, "404", "Not Found", "Could not find the path specified:", path);
        goto cleanup;
    }

    fputs("HTTP/1.0 200 OK\r\n", stream);
    fputs("Content-Type: ", stream);
    fputs(contentType, stream);
    fputs("\r\n", stream);

    fputs("\r\n", stream);

    if (startsWith("image", contentType))
        while (!feof(page))
            fputc(fgetc(page), stream); //special handling for images
    else
        while (!feof(page))
        {
            getline(&line, &size, page);
            fputs(line, stream);
        }

cleanup:
    free(line);

    if (page)
        fclose(page);
}

//Checks if str starts with smol
//returns 1 if true
//0 if false
int startsWith(const char *smol, const char *str)
{
    int i = 0;
    for (i = 0; i < strlen(smol); i++)
    {
        if (smol[i] != str[i])
            return 0;
    }
    return 1;
}

//Response to bad HTTP requests, needs stream, error number in errnum, response title, response body, and path
//path and httpBody are optional (pass NULL to them)
void badHTTP(FILE *stream, char *errnum, char *title, char *httpBody, char *path)
{
    fflush(stream);

    char msg[BUFFSIZE] = "HTTP/1.0 "; //No need to test buffer because message if code generated, not user input.
    strlcat(msg, errnum, BUFFSIZE);
    strlcat(msg, " ", BUFFSIZE);
    strlcat(msg, title, BUFFSIZE);
    strlcat(msg, "\r\n", BUFFSIZE);

    fputs(msg, stream);
    fputs("Content-Type: text/plain\r\n", stream);
    fputs("\r\n", stream);

    if (httpBody)
    {
        fputs(httpBody, stream);
        fputs("\r\n", stream);
    }

    if (path)
    {
        fputs("Path: ", stream);
        fputs(path, stream);
        fputs("\r\n", stream);
    }
}
