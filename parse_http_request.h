/**
 * Created by Alexandru Blinda.
 */
#include <pthread.h>

#define HTTP_REQUEST_DELIMITER "\r\n"
#define HTTP_VERSION "HTTP/1.1"
#define HTTP_OK_CODE "200"
#define HTTP_OK_REASON_PHRASE "OK"
#define HTTP_BAD_REQUEST_CODE "400"
#define HTTP_BAD_REQUEST_PHRASE "Bad Request"
#define HTTP_NOT_FOUND_CODE "404"
#define HTTP_NOT_FOUND_PHRASE "Not Found"
#define HTTP_METHOD_NOT_ALLOWED_CODE "405"
#define HTTP_METHOD_NOT_ALLOWED_PHRASE "Method Not Allowed"
#define HTTP_INTERNAL_SERVER_ERROR_CODE "500"
#define HTTP_INTERNAL_SERVER_ERROR_PHRASE "Internal Server Error"
#define HTTP_NOT_IMPLEMENTED_CODE "501"
#define HTTP_NOT_IMPLEMENTED_PHRASE "Not Implemented"
#define HTTP_VERSION_NOT_SUPPORTED_CODE "505"
#define HTTP_VERSION_NOT_SUPPORTED_PHRASE "Version Not Supported"
#define HTTP_REQUEST_ENTITY_TOO_LARGE_CODE "413"
#define HTTP_REQUEST_ENTITY_TOO_LARGE_PHRASE "Request Entity Too Large"
#define HTTP_CONTENT_LENGTH_HEADER "Content-Length"
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

typedef struct http_response_block {

    /**
     * Struct to keep the components of a status-line: Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
     */
    struct http_status_line {

        char* http_version;
        char* http_status_code;
        char* http_reason_phrase;
    } http_status_line;
    /** Headers */
    char** http_response_headers;
    char* http_response_message_body;
} http_response_block_t;

typedef enum request_state {
    reading_http_request_line = 0,
    reading_http_request_headers = 1,
    reading_http_request_body_message = 2,
} request_state_t;

char* split_string(char*, const char*, char**);
int string_count(char*, const char*);

int parse_http_request(char*, int, char**, pthread_mutex_t*);
void parse_http_request_line(char*, http_request_block_t*);
int validate_http_request_line(http_request_block_t*, http_response_block_t*, pthread_mutex_t*);
int validate_http_version(http_request_block_t*, http_response_block_t*, pthread_mutex_t*);
int validate_http_method(http_request_block_t*, http_response_block_t*, pthread_mutex_t*);
int parse_http_request_header(char*, http_request_block_t*, int*);
int validate_http_request_headers(http_request_block_t*, http_response_block_t*, int*, pthread_mutex_t*);

int handle_http_request(http_request_block_t*, http_response_block_t*, int*, pthread_mutex_t*);
int parse_http_response(http_response_block_t*, char**, int*);

void bad_request(http_response_block_t*, pthread_mutex_t*);
void version_not_supported(http_response_block_t*, pthread_mutex_t*);
void method_not_allowed(http_response_block_t*, pthread_mutex_t*);
void not_implemented(http_response_block_t*, pthread_mutex_t*);
void not_found(http_response_block_t*, pthread_mutex_t*);
void internal_server_error(http_response_block_t*, pthread_mutex_t*);
void request_entity_too_large(http_response_block_t*, pthread_mutex_t*);
void ok(http_response_block_t*, char* filename, pthread_mutex_t*);

void clean_http_request_block(http_request_block_t*);
void clean_http_response_block(http_response_block_t*, int*);

int add_content_length_header(http_response_block_t*, int*);