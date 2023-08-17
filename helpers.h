#ifndef _HELPERS_
#define _HELPERS_

#include "parson.h"

// Macros
#define BUFLEN 4096
#define LINELEN 1000

#define HOST "34.254.242.81"
#define PORT 8080
#define HOST_PORT "34.254.242.81:8080"

// Error checking macro
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

// Check if a string matches the desired format, based on type
// type = 0 -> [0-9]
// type = 1 -> [0-9], [a-z], [A-Z], punctuation
// type = 2 -> same as type 1 + spaces
int check_input(char *s, int type);

// Read a line from stdin and check if it matches the desired format
char *read_input(int type, int *ok_input);

// Extract the error name from the JSON received in server response
const char *extract_err(char *response);

// Extract JSON payload from server response
JSON_Value *extract_payload(char *response);

// Extract cookie from server response
char *extract_cookie(char *response);

// Extract HTTP response status code
int parse_code(char *response);

// Show the current error
void error(const char *msg);

// Add a line to a string message
void compute_message(char *message, const char *line);

// Open a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// Close a server connection on socket sockfd
void close_connection(int sockfd);

// Send a message and receive a response from the server
// Check if the server closed, reopen and resend if necessary
char *send_and_recv_response(int sockfd, char *send);

// Send a message to a server
void send_to_server(int sockfd, char *message);

// Receive and return the message from a server
char *receive_from_server(int sockfd);

#endif
