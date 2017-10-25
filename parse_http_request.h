/**
 * Created by Alexandru Blinda.
 */


/**
 * Struct to keep the components of a request
 */
typedef struct http_request_block {

    /**
     * Struct to keep the components of a request line
     */
    struct http_request_line {

        char* http_method;
        char* http_request_uri;
        char* http_version;
    } http_request_line;
    char** http_request_headers;
    char* http_request_message_body;
} http_request_block_t;

int parse_http_request(char*, int request_size, char*);
char* split_string(char*, const char*, char**);
int string_count(char*, const char*);