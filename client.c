#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <sys/poll.h>

#include "parson.h"
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    char *cookie = NULL;
    char *token = NULL;

    // Open server connection
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    // Poll stdin and server socket
    struct pollfd poll_fds[2];
    int num_clients = 2;

    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = 0;
    poll_fds[1].events = POLLIN;

    while (1)
    { // Wait infinitely for event
        int rc = poll(poll_fds, num_clients, -1);
        DIE(rc < 0, "poll");

        // If client received data from server
        if (poll_fds[0].revents && POLLIN)
        { // Read data sent
            char buf[BUFSIZ];
            memset(buf, 0, BUFSIZ);
            rc = recv(sockfd, &buf, BUFSIZ, 0);
            DIE(rc < 0, "recv");

            // Server closed connection. Reopen
            if (rc == 0)
            {
                close_connection(sockfd);
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            }
        }

        // If client received data from stdin
        else if (poll_fds[1].revents && POLLIN)
        {
            int ok_input = 1;

            // Read request
            char *request = read_input(1, &ok_input);
            if (!ok_input)
                continue;

            if (!strcmp(request, "register"))
            {
                printf("username=");
                char *username = read_input(1, &ok_input);
                if (!ok_input)
                    continue;

                printf("password=");
                char *password = read_input(1, &ok_input);
                if (!ok_input)
                    continue;

                // Create JSON structure with username and password as string
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                json_object_set_string(root_object, "username", username);
                json_object_set_string(root_object, "password", password);
                char *json_str = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(HOST_PORT, "/api/v1/tema/auth/register",
                                               "application/json", json_str, NULL, NULL);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 201)
                    printf("User succesfully registered!\n");
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "login"))
            {
                printf("username=");
                char *username = read_input(1, &ok_input);
                if (!ok_input)
                    continue;

                printf("password=");
                char *password = read_input(1, &ok_input);
                if (!ok_input)
                    continue;

                // Create JSON structure with username and password as string
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                json_object_set_string(root_object, "username", username);
                json_object_set_string(root_object, "password", password);
                char *json_str = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(HOST_PORT, "/api/v1/tema/auth/login",
                                               "application/json", json_str, NULL, NULL);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                {
                    printf("User successfully logged in.\n");
                    // Save cookie from server response
                    cookie = extract_cookie(response);
                }
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "enter_library"))
            { // Check if cookie exists (= user logged in)
                if (!cookie)
                {
                    printf("Login and try again.\n");
                    continue;
                }

                message = compute_get_delete_request("GET", HOST_PORT,
                                                     "/api/v1/tema/library/access", cookie, NULL);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                {
                    printf("Access granted to the library.\n");

                    // Save jwt token from server response
                    JSON_Value *json = extract_payload(response);
                    token = json_object_dotget_string(json_value_get_object(json), "token");
                }
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "get_books"))
            { // Check if token exists (= user has access to library)
                if (!token)
                {
                    printf("Request access to the library first.\n");
                    continue;
                }

                message = compute_get_delete_request("GET", HOST_PORT,
                                                     "/api/v1/tema/library/books", NULL, token);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                { // Extract JSON payload from server response
                    JSON_Value *json = extract_payload(response);
                    JSON_Array *books_array = json_value_get_array(json);
                    size_t count = json_array_get_count(books_array);

                    for (size_t i = 0; i < count; i++)
                    { // Extract the id and title from each JSON object and print
                        JSON_Object *book = json_array_get_object(books_array, i);
                        int id = json_object_get_number(book, "id");
                        const char *title = json_object_get_string(book, "title");
                        printf("id: %d\ntitle: %s\n", id, title);
                    }

                    json_value_free(json);
                }
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "get_book"))
            { // Check if token exists (= user has access to library)
                if (!token)
                {
                    printf("Request access to the library first.\n");
                    continue;
                }

                printf("id=");
                char *_id = read_input(0, &ok_input);
                if (!ok_input)
                    continue;

                // Create URL using input id
                int id = atoi(_id);
                char url[LINELEN];
                sprintf(url, "/api/v1/tema/library/books/%d", id);

                message = compute_get_delete_request("GET", HOST_PORT, url, NULL, token);
                response = send_and_recv_response(sockfd, message);

                int code = parse_code(response);
                // Check code
                if (code == 200)
                { // Extract JSON payload from server response and then each field of the book
                    JSON_Value *json = extract_payload(response);
                    const char *title = json_object_dotget_string(json_value_get_object(json), "title");
                    const char *author = json_object_dotget_string(json_value_get_object(json), "author");
                    const char *publisher = json_object_dotget_string(json_value_get_object(json), "publisher");
                    const char *genre = json_object_dotget_string(json_value_get_object(json), "genre");
                    double page_count = json_object_dotget_number(json_value_get_object(json), "page_count");

                    printf("title=%s\nauthor=%s\npublisher=%s\ngenre=%s\npage_count=%d\n",
                           title, author, publisher, genre, (int)page_count);

                    json_value_free(json);
                }
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "add_book"))
            { // Check if token exists (= user has access to library)
                if (!token)
                {
                    printf("Request access to the library first.\n");
                    continue;
                }

                printf("title=");
                char *title = read_input(2, &ok_input);
                if (!ok_input)
                    continue;

                printf("author=");
                char *author = read_input(2, &ok_input);
                if (!ok_input)
                    continue;

                printf("publisher=");
                char *publisher = read_input(2, &ok_input);
                if (!ok_input)
                    continue;

                printf("genre=");
                char *genre = read_input(2, &ok_input);
                if (!ok_input)
                    continue;

                printf("page_count=");
                char *_page_count = read_input(0, &ok_input);
                if (!ok_input)
                    continue;

                int page_count = atoi(_page_count);

                // Create JSON Onject (with book values) as string
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                json_object_set_string(root_object, "title", title);
                json_object_set_string(root_object, "author", author);
                json_object_set_string(root_object, "publisher", publisher);
                json_object_set_string(root_object, "genre", genre);
                json_object_set_number(root_object, "page_count", page_count);

                char *json_obj = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(HOST_PORT, "/api/v1/tema/library/books",
                                               "application/json", json_obj, NULL, token);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                    printf("Book added.\n");
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "delete_book"))
            { // Check if token exists (= user has access to library)
                if (!token)
                {
                    printf("Request access to the library first.\n");
                    continue;
                }

                printf("id=");
                char *_id = read_input(0, &ok_input);
                if (!ok_input)
                    continue;

                // Create URL using input id
                int id = atoi(_id);
                char url[LINELEN];
                sprintf(url, "/api/v1/tema/library/books/%d", id);

                message = compute_get_delete_request("DELETE", HOST_PORT, url, NULL, token);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                    printf("Deleted book.\n");
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "logout"))
            { // Check if cookie exists (= user logged in)
                if (!cookie)
                {
                    printf("You are not logged in.\n");
                    continue;
                }

                message = compute_get_delete_request("GET", HOST_PORT,
                                                     "/api/v1/tema/auth/logout", cookie, NULL);
                response = send_and_recv_response(sockfd, message);

                // Check code
                int code = parse_code(response);
                if (code == 200)
                {
                    printf("Logged out.\n");

                    // Free and set to NULL cookie and token
                    free(cookie);
                    cookie = NULL;
                    if (token)
                    {
                        free(token);
                        token = NULL;
                    }
                }
                else if (code / 100 == 4 || code / 100 == 5)
                    printf("Error code %d: %s\n", code, extract_err(response));
            }
            else if (!strcmp(request, "exit"))
            {
                break;
            }
            else
            {
                printf("Unrecognized command!\n");
                continue;
            }
        }
    }

    // Close connection with server
    close_connection(sockfd);

    return 0;
}
