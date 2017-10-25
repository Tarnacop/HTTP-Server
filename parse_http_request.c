/**
 * Created by Alexandru Blinda.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse_http_request.h"

#define HTTP_REQUEST_DELIMITER "\r\n"

int parse_http_request(char* request, int request_size, char* response) {

    /** Count the number of CLRF's in the request obtained */
    int crlf_number = string_count(request, HTTP_REQUEST_DELIMITER);

    /** Must have at least 3 splits: Request-Line, Header and Message Body */
    /**
     * Request    = Request-Line (ENDS WITH A CRLF)
     *              *(( general-header
     *              | request-header
     *              | entity-header ) CRLF)
     *              CRLF
     *              [ message-body ]
     */
    if(crlf_number < 3) {

        return -1;
    }

    http_request_block_t hrb_t;
    memset(&hrb_t, '\0', sizeof(http_request_block_t));

    char* saveptr = NULL;
    char* request_line = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);

    /**
     * First part of the request is the request line with 3 different parts
     * Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
     */
    char* request_line_saveptr = NULL;
    hrb_t.http_request_line.http_method = split_string(request_line, " ", &request_line_saveptr);
    if(hrb_t.http_request_line.http_method == NULL || strlen(hrb_t.http_request_line.http_method) == 0) {

        // TODO - further checks?
        return -1;
    }
    hrb_t.http_request_line.http_request_uri = split_string(request_line, " ", &request_line_saveptr);
    if(hrb_t.http_request_line.http_request_uri == NULL || strlen(hrb_t.http_request_line.http_request_uri) == 0) {

        // TODO - further checks?
        return -1;
    }
    hrb_t.http_request_line.http_version = split_string(request_line, " ", &request_line_saveptr);
    if(hrb_t.http_request_line.http_version == NULL || strlen(hrb_t.http_request_line.http_version) == 0) {

        // TODO - further checks?
        return -1;
    }

    /** Second part is the Headers Part. For HTTP/1.1 Host header must be included */
    int count_headers = 0;
    /**
     * We know the numbers of CLRF so we can conclude the number of Headers
     * 2 CLRF come from the Request - Line and from the message body => the rest are header CLRF
     */
    while(count_headers < crlf_number - 2) {

        count_headers++;
        if(hrb_t.http_request_headers == NULL) {

            hrb_t.http_request_headers = malloc(sizeof(char *));
            *(hrb_t.http_request_headers) = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);
        } else {

            hrb_t.http_request_headers = realloc(hrb_t.http_request_headers, count_headers * sizeof(char *));
            *(hrb_t.http_request_headers + count_headers - 1) = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);
        }
    }

    /**
     * Clearly the last CLRF is for a message body so
     */
    hrb_t.http_request_message_body = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);
    /** Message Body can be empty */
    if(hrb_t.http_request_message_body == NULL) {

        return -1;
    }

    printf("HTTP Method: %s\n", hrb_t.http_request_line.http_method);
    printf("HTTP Request-URI: %s\n", hrb_t.http_request_line.http_request_uri);
    printf("HTTP Version: %s\n", hrb_t.http_request_line.http_version);
    for(int i = 0; i < count_headers; i++) {

        printf("HTTP Headers: %s\n", *(hrb_t.http_request_headers + i));
    }
    printf("HTTP Body: %s\n", hrb_t.http_request_message_body);

    return 0;
}

/** Kind of a strtok, but working with strings as delimiter */
char* split_string(char* string, const char* delimiter, char** saveptr) {

    /** Get the length of the delimiter string */
    int delimiter_len = strlen(delimiter);

    if(*saveptr != NULL) {

        string = *saveptr;
    }
    /** Occurrence used to store the address of the first occurrence*/
    char* occurrence = strstr(string, delimiter);
    if(occurrence != NULL) {

        memset(occurrence, '\0', delimiter_len);
        *saveptr = occurrence + delimiter_len;
        return string;
    }
    /** If there is no delimiter appearing, but we have not reached the end of the string, return the remaining string */
    if(*string != '\0') {

        *saveptr = string + strlen(string) + 1; /** +1 to accommodate for the \0 at the end */
        return string;
    }

    return NULL;
}

/** Function to count the number of occurrences of a string in another string */
int string_count(char* string, const char* delimiter) {

    int delimiter_len = strlen(delimiter);
    char* helper_pointer = string;
    int counter = 0;
    while((helper_pointer=strstr(helper_pointer, delimiter)) != NULL) {

        counter++;
        helper_pointer = helper_pointer + delimiter_len;
    }

    return counter;
}