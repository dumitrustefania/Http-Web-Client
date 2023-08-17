#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET/DELETE request string (cookie and token
// can be set to NULL if not needed)
char *compute_get_delete_request(char *type, char *host, char *url,
								 char *cookie, char *token);

// computes and returns a POST request string (cookie and token
// can be set to NULL if not needed)
char *compute_post_request(char *host, char *url, char *content_type, char *body_data,
						   char *cookie, char *token);

#endif
