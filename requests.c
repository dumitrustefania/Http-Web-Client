#include <stdlib.h> /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_delete_request(char *type, char *host, char *url, char *cookie, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //  Write the method name (GET/DELETE), URL and protocol type
    sprintf(line, "%s %s HTTP/1.1", type, url);
    compute_message(message, line);

    // Add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add cookie
    if (cookie != NULL)
    {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    // Add jwt token
    if (token != NULL)
    {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Add final new line
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type, char *json_obj,
                           char *cookie, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Specify content type
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Compute and write content length
    sprintf(line, "Content-Length: %lu", strlen(json_obj));
    compute_message(message, line);

    // Add cookie
    if (cookie != NULL)
    {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    // Add jwt token
    if (token != NULL)
    {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Add new line at end of header
    compute_message(message, "");

    // Add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, json_obj);

    // Add final new line
    compute_message(message, "");

    free(line);
    return message;
}
