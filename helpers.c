#include <stdlib.h> /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "parson.h"
#include "helpers.h"
#include "buffer.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

int check_input(char *s, int type)
{
    int ok = 1;

    if(!strcmp(s, "") || !strcmp(s, "\n"))
        ok = 0;

    for (int i = 0; i < strlen(s) && ok; i++)
    {
        if (isdigit(s[i]))
            continue;
        else if (isalpha(s[i]) || ispunct(s[i]))
        {
            if (type == 0)
            {
                ok = 0;
                break;
            }
        }
        else if (s[i] == ' ')
        {
            if (type < 2)
            {
                ok = 0;
                break;
            }
        }
        else
        {
            ok = 0;
            break;
        }
    }

    if (!ok)
        printf("Bad input.\n");

    return ok;
}

char *read_input(int type, int *ok_input)
{
    char *str = malloc(LINELEN);
    fgets(str, LINELEN, stdin);
    str[strlen(str) - 1] = '\0';

    *ok_input = check_input(str, type);

    return str;
}

const char *extract_err(char *response)
{
    JSON_Value *json = extract_payload(response);
    const char *err = json_object_dotget_string(json_value_get_object(json), "error");
    return err;
}

JSON_Value *extract_payload(char *response)
{
    char *payload = strstr(response, "\r\n\r\n") + 1;
    JSON_Value *json = json_parse_string(payload);
    return json;
}

char *extract_cookie(char *response)
{
    char *line = strstr(response, "Set-Cookie");
    char *cookie = malloc(LINELEN);
    line = strstr(line, " ");
    strcpy(cookie, strtok(line, ";"));

    return cookie;
}

int parse_code(char *response)
{
    int code = (response[9] - '0') * 100 + (response[10] - '0') * 10 + (response[11] - '0');
    return code;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    // Create a socket to connect the client with the server
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Fill serv_addr with the data received as parameter
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    // Connect the socket
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

char *send_and_recv_response(int sockfd, char *send)
{
    send_to_server(sockfd, send);
    char *response = receive_from_server(sockfd);

    // If response is empty, it means that the server
    // closed the connection
    if (!strcmp(response, ""))
    {   // Reopen connection
        close_connection(sockfd);
        sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
        
        // Resend message
        send_to_server(sockfd, send);
        response = receive_from_server(sockfd);
    }

    return response;
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    { // Repeat until the whole message is sent
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0)
        {
            error("ERROR writing message to socket");
        }

        if (bytes == 0)
        {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do
    {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0)
        {
            error("ERROR reading response from socket");
        }

        if (bytes == 0)
        {
            break;
        }

        buffer_add(&buffer, response, (size_t)bytes);

        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0)
        {
            header_end += HEADER_TERMINATOR_SIZE;

            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);

            if (content_length_start < 0)
            {
                continue;
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t)header_end;

    while (buffer.size < total)
    {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0)
        {
            error("ERROR reading response from socket");
        }

        if (bytes == 0)
        {
            break;
        }

        buffer_add(&buffer, response, (size_t)bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}