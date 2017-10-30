/**
 * Created by Alexandru Blinda.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>

#include "parse_http_request.h"

int parse_http_request(char* request, int request_size, char** response, pthread_mutex_t* lock) {

    http_request_block_t hrb_t;
    memset(&hrb_t, '\0', sizeof(http_request_block_t));

    http_response_block_t hrespb_t;
    memset(&hrespb_t, '\0', sizeof(http_response_block_t));

    int header_index = 0;
    int response_header_index = 0;
    if(request_size == 8 * 1024) {

        request_entity_too_large(&hrespb_t, lock);
        add_content_length_header(&hrespb_t, &response_header_index);
        parse_http_response(&hrespb_t, response, &response_header_index);
        clean_http_request_block(&hrb_t);
        clean_http_response_block(&hrespb_t, &response_header_index);
        return 0;
    }

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

        bad_request(&hrespb_t, lock);
        add_content_length_header(&hrespb_t, &response_header_index);
        parse_http_response(&hrespb_t, response, &response_header_index);
        clean_http_request_block(&hrb_t);
        clean_http_response_block(&hrespb_t, &response_header_index);
        return 0;
    }

    char* saveptr = NULL;
    char* request_line = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);

    /**
     * First part of the request is the request line with 3 different parts
     * Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
     */
    parse_http_request_line(request_line, &hrb_t);
    if(validate_http_request_line(&hrb_t, &hrespb_t, lock) < 0) {

        add_content_length_header(&hrespb_t, &response_header_index);
        parse_http_response(&hrespb_t, response, &response_header_index);
        clean_http_request_block(&hrb_t);
        clean_http_response_block(&hrespb_t, &response_header_index);
        return 0;
    }

    /** Second part is the Headers Part. For HTTP/1.1 Host header must be included */
    /**
     * We know the numbers of CLRF so we can conclude the number of Headers
     * 2 CLRF come from the Request - Line and from the message body => the rest are header CLRF
     */
    while(header_index < crlf_number - 2) {

        if(parse_http_request_header(split_string(request, HTTP_REQUEST_DELIMITER, &saveptr), &hrb_t, &header_index) < 0) {

            internal_server_error(&hrespb_t, lock);
            add_content_length_header(&hrespb_t, &response_header_index);
            parse_http_response(&hrespb_t, response, &response_header_index);
            clean_http_request_block(&hrb_t);
            clean_http_response_block(&hrespb_t, &response_header_index);
            return 0;
        }
    }

    if(validate_http_request_headers(&hrb_t, &hrespb_t, &header_index, lock) < 0) {

        add_content_length_header(&hrespb_t, &response_header_index);
        parse_http_response(&hrespb_t, response, &response_header_index);
        clean_http_request_block(&hrb_t);
        clean_http_response_block(&hrespb_t, &response_header_index);
        return 0;
    }

    /**
     * Clearly the last CLRF is for a message body so
     */
    hrb_t.http_request_message_body = split_string(request, HTTP_REQUEST_DELIMITER, &saveptr);

    /** Message Body can be empty */

    printf("HTTP Method: %s\n", hrb_t.http_request_line.http_method);
    printf("HTTP Request-URI: %s\n", hrb_t.http_request_line.http_request_uri);
    printf("HTTP Version: %s\n", hrb_t.http_request_line.http_version);
    for(int i = 0; i < header_index; i++) {

        printf("HTTP Headers: %s\n", *(hrb_t.http_request_headers + i));
    }
    printf("HTTP Body: %s\n", hrb_t.http_request_message_body);

    handle_http_request(&hrb_t, &hrespb_t, &response_header_index, lock);

    parse_http_response(&hrespb_t, response, &response_header_index);

    clean_http_request_block(&hrb_t);
    clean_http_response_block(&hrespb_t, &response_header_index);

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

void parse_http_request_line(char* request_line, http_request_block_t* request_block) {

    char* saveptr = NULL;
    char* aux_char = NULL; /** Used to iterate through the strtok_r */
    /**
     * We know the request line looks like Request-Line = Method SP Request-URI SP HTTP-Version CRLF
     * So we strtok_r over the delimiter SP
     * We know it should have only 3 splits in order to get the elements
     */
    int no_of_splits = 0;
    while((aux_char = strtok_r(request_line, " ", &saveptr)) != NULL) {

        no_of_splits++;
        request_line = NULL; /** Necessary for strotk_r to work ... */
        /** The element from the first split is the Method */
        switch(no_of_splits) {

            /** Case 1 is Method */
            case 1:
                request_block->http_request_line.http_method = aux_char;
                break;
            /** Case 2 is Request-Uri */
            case 2:
                request_block->http_request_line.http_request_uri = aux_char;
                break;
            /** Case 3 is HTTP-Version */
            case 3:
                request_block->http_request_line.http_version = aux_char;
                break;
            default:
                return;
        }
    }
}

int parse_http_request_header(char* header_line, http_request_block_t* request_block, int* index) {

    int aux_index = *index;

    /** No header saved so we malloc memory */
    if(aux_index == 0) {

        request_block->http_request_headers = malloc(sizeof(char*));
        if(request_block->http_request_headers == NULL) {

            return -1;
        }
        *(request_block->http_request_headers) = header_line;
    }
    else {

        char** aux_pointer = realloc(request_block->http_request_headers, (aux_index + 1) * sizeof(char*));
        if(aux_pointer == NULL) {

            return -1;
        }
        request_block->http_request_headers = aux_pointer;
        *(request_block->http_request_headers + aux_index) = header_line;
    }

    aux_index++;
    *index = aux_index;
    return 0;
}

int validate_http_request_headers(http_request_block_t* request_block, http_response_block_t* response_block,
                                  int* max_index, pthread_mutex_t* lock) {

    /** According to HTTP/1.1 every request must contain the Host header */
    int aux_max_index = *max_index;
    for(int i = 0; i < aux_max_index; i++) {

        if(strstr(*(request_block->http_request_headers + i), "Host:") != NULL) {

            return 0;
        }
    }

    bad_request(response_block, lock);
    return -1;
}

int validate_http_request_line(http_request_block_t* request_block, http_response_block_t* response_block, pthread_mutex_t* lock) {

    /**
     * Step 1 - Check that every element of the request_line is not null or empty
     * If it is, return and create bad request
     */
    if(request_block->http_request_line.http_method == NULL
       || strlen(request_block->http_request_line.http_method) == 0) {

        bad_request(response_block, lock);
        return -1; /** -1 in our case indicates that we should write the response */
    }

    if(request_block->http_request_line.http_request_uri == NULL
       || strlen(request_block->http_request_line.http_request_uri) == 0) {

        bad_request(response_block, lock);
        return -1;
    }

    if(request_block->http_request_line.http_version == NULL
       || strlen(request_block->http_request_line.http_version) == 0) {

        bad_request(response_block, lock);
        return -1;
    }

    /**
     * Step 2 - Check if the http version of the request is valid
     * If it is an invalid format, 400
     * If it is a valid format, but not supported, 505
     */

    if(validate_http_version(request_block, response_block, lock) < 0) {

        return -1;
    }

    /**
     * Step 3 - Check if the http method is recognised
     * If the method is one of the methods provided by HTTP/1.1 but not supported to the resource, return 405
     * If the method is not recognised by HTTP/1.1 return 501
     */
    if(validate_http_method(request_block, response_block, lock) < 0) {

        return -1;
    }

    return 0;
}

int validate_http_version(http_request_block_t* request_block, http_response_block_t* response_block, pthread_mutex_t* lock) {

    /** HTTP version must be of form HTTP/a.b where a and b are numbers */
    /** Check if it starts with HTTP */
    if(strncmp(request_block->http_request_line.http_version, "HTTP", 4) != 0) {

        bad_request(response_block, lock);
        return -1;
    }

    /** Check if the 5-th char is a / */
    if(request_block->http_request_line.http_version[4] != '/') {

        bad_request(response_block, lock);
        return -1;
    }

    /** Check if the following element is a number */
    int index = 5;
    while((request_block->http_request_line.http_version[index] != '.')
          && (request_block->http_request_line.http_version[index] != '\0')) {

        if(!isdigit(request_block->http_request_line.http_version[index])) {

            bad_request(response_block, lock);
            return -1;
        }
        index++;
    }

    /** Check that the first number ends with . */
    if(request_block->http_request_line.http_version[index] != '.') {

        bad_request(response_block, lock);
        return -1;
    }

    index++;

    /** Check that the http version line ends with a number */
    while(request_block->http_request_line.http_version[index] != '\0') {

        if(!isdigit(request_block->http_request_line.http_version[index])) {

            bad_request(response_block, lock);
            return -1;
        }
        index++;
    }

    /** If the previous checks are passed, check that the version is HTTP/1.1 */
    if(strcmp(request_block->http_request_line.http_version, HTTP_VERSION) != 0) {

        version_not_supported(response_block, lock);
        return -1;
    }

    return 0;
}

int validate_http_method(http_request_block_t* request_block, http_response_block_t* response_block, pthread_mutex_t* lock) {

    short method_encoding = 0;
    if(strcmp(request_block->http_request_line.http_method, "GET") == 0) {

        method_encoding = 1;
    } else if(strcmp(request_block->http_request_line.http_method, "HEAD") == 0) {

        method_encoding = 2;
    } else if(strcmp(request_block->http_request_line.http_method, "POST") == 0) {

        method_encoding = 3;
    } else if(strcmp(request_block->http_request_line.http_method, "PUT") == 0) {

        method_encoding = 4;
    } else if(strcmp(request_block->http_request_line.http_method, "OPTIONS") == 0) {

        method_encoding = 5;
    } else if(strcmp(request_block->http_request_line.http_method, "DELETE") == 0) {

        method_encoding = 6;
    } else if(strcmp(request_block->http_request_line.http_method, "TRACE") == 0) {

        method_encoding = 7;
    } else if(strcmp(request_block->http_request_line.http_method, "CONNECT") == 0) {

        method_encoding = 8;
    }

    switch(method_encoding) {

        /**
         * If it is a GET, try and open the file provided and return 200 OK response
         * HEAD is GET without actually sending the data
         * If it is any other method described in HTTP RFC, return 405 response
         * If it is a method not described in HTTP RFC, return 501
         */
        /** GET */
        case 1:
            // do nothing
            break;
        /** HEAD */
        case 2:
            // do nothing
            break;
        /** POST, PUT ... */
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            method_not_allowed(response_block, lock);
            return -1;
        /** Any undeclared method */
        default:
            not_implemented(response_block, lock);
            return -1;
    }

    return 0;
}

int handle_http_request(http_request_block_t* request_block,  http_response_block_t* response_block,
                        int* max_header_index, pthread_mutex_t* lock) {

    if(strcmp(request_block->http_request_line.http_method, "GET") == 0
       || strcmp(request_block->http_request_line.http_method, "HEAD") == 0) {

        if(strcmp(request_block->http_request_line.http_request_uri, "/") == 0) {

            ok(response_block, "index.html", lock);
        } else {

            ok(response_block, &request_block->http_request_line.http_request_uri[1], lock);
        }
        add_content_length_header(response_block, max_header_index);

        if(strcmp(request_block->http_request_line.http_method, "HEAD") == 0) {

            if(response_block->http_response_message_body != NULL) {

                free(response_block->http_response_message_body);
            }
            response_block->http_response_message_body = strdup("\0");
        }
    } else {

        internal_server_error(response_block, lock);
    }

    return 0;
}

int parse_http_response(http_response_block_t* response_block, char** response, int* max_header_index) {

    int size = 0;
    if(response_block->http_status_line.http_version != NULL) {

        size += strlen(response_block->http_status_line.http_version) + 1; /** +1 for space */
    }
    if(response_block->http_status_line.http_status_code != NULL) {

        size += strlen(response_block->http_status_line.http_version) + 1; /** +1 for space */
    }
    if(response_block->http_status_line.http_reason_phrase != NULL) {

        size += strlen(response_block->http_status_line.http_reason_phrase) + 2; /** +2 for CRLF */
    }
    if(response_block->http_response_headers != NULL) {

        for(int i = 0; i < *max_header_index; i++) {

            size += strlen(*(response_block->http_response_headers + i)) + 2; /** +2 for CRLF */
        }

        size += 2; /** For the CRLF between headers and message body */
    }
    if(response_block->http_response_message_body != NULL) {

        size += strlen(response_block->http_response_message_body);
    }

    *response = malloc( (size + 1) * sizeof(char)); /** +1 for \0 byte in the end */
    /** If we cannot allocate memory, try again after 10 mins of sleeping */
    while(response == NULL) {

        sleep(10);
        *response = malloc(size * sizeof(char));
    }

    /** Response      = Status-Line
                       *(( general-header
                        | response-header
                        | entity-header ) CRLF)
                       CRLF
                       [ message-body ]

       Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
      */

    strcpy(*response, response_block->http_status_line.http_version);
    *response = strcat(*response, " ");
    strcat(strcat(*response, response_block->http_status_line.http_status_code), " ");
    strcat(strcat(*response, response_block->http_status_line.http_reason_phrase), "\r\n");
    for(int i = 0; i < *max_header_index; i++) {

        strcat(*response, *(response_block->http_response_headers + i));
        strcat(*response, "\r\n");
    }
    strcat(*response, "\r\n");
    if(response_block->http_response_message_body != NULL) {

        strcat(*response, response_block->http_response_message_body);
    }

    return 0;
}

void bad_request(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_BAD_REQUEST_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_BAD_REQUEST_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/400.html", "r");
    if(file == NULL) {
        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);

}

void version_not_supported(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_VERSION_NOT_SUPPORTED_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_VERSION_NOT_SUPPORTED_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/505.html", "r");
    if(file == NULL) {
        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void method_not_allowed(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_METHOD_NOT_ALLOWED_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_METHOD_NOT_ALLOWED_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/405.html", "r");
    if(file == NULL) {

        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void not_implemented(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_NOT_IMPLEMENTED_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_NOT_IMPLEMENTED_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/501.html", "r");
    if(file == NULL) {

        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void not_found(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_NOT_FOUND_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_NOT_FOUND_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/404.html", "r");
    if(file == NULL) {

        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void internal_server_error(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_INTERNAL_SERVER_ERROR_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_INTERNAL_SERVER_ERROR_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/500.html", "r");
    if(file == NULL) {
        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void request_entity_too_large(http_response_block_t* http_response, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_REQUEST_ENTITY_TOO_LARGE_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_REQUEST_ENTITY_TOO_LARGE_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen("html_pages/413.html", "r");
    if(file == NULL) {

        pthread_mutex_unlock(lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*size);
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);
        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

void ok(http_response_block_t* http_response, char* filename, pthread_mutex_t* lock) {

    /** Status-Line */
    http_response->http_status_line.http_version = HTTP_VERSION;
    http_response->http_status_line.http_status_code = HTTP_OK_CODE;
    http_response->http_status_line.http_reason_phrase = HTTP_OK_REASON_PHRASE;

    pthread_mutex_lock(lock);
    FILE* file = fopen(filename, "r");
    if(file == NULL) {

        pthread_mutex_unlock(lock);
        not_found(http_response, lock);
        return;
    } else {

        /** Obtain file size */
        fseek (file , 0 , SEEK_END);
        long size = ftell (file);
        rewind (file);

        /** Allocate memory to contain the whole file */
        /** +1 So we can add an ending null byte */
        http_response->http_response_message_body = (char*) malloc (sizeof(char)*(size + 1));
        if (http_response->http_response_message_body == NULL) {

            fclose(file);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        size_t result = fread(http_response->http_response_message_body, sizeof(char), size, file);

        if(result != size) {

            fclose(file);
            free(http_response->http_response_message_body);
            pthread_mutex_unlock(lock);
            internal_server_error(http_response, lock);
            return;
        }
        *(http_response->http_response_message_body + size) = '\0'; /** Make it a string */
        fclose(file);
    }
    pthread_mutex_unlock(lock);
}

int add_content_length_header(http_response_block_t* http_response, int* header_index) {

    int aux_header_index = *header_index;
    int value = 0;
    if(http_response->http_response_message_body != NULL) {

        value = strlen(http_response->http_response_message_body);
    }
    int aux_value = value; /** We will destroy this number */
    int counter = 0; /** Digit number counter */
    do {
        aux_value = aux_value / 10;
        counter++;
    }while(aux_value != 0);

    char string_value[counter + 1];
    memset(string_value, '\0', counter + 1);
    sprintf(string_value, "%d", value);
    if(aux_header_index == 0) {

        http_response->http_response_headers = malloc(sizeof(char*));
        if(http_response->http_response_headers == NULL) {

            return -1;
        }
        int size = strlen(HTTP_CONTENT_LENGTH_HEADER) + 2 + strlen(string_value); /** +2 for ": " */

        *(http_response->http_response_headers) = malloc(size * sizeof(char));
        if(*(http_response->http_response_headers) == NULL) {

            free(http_response->http_response_headers);
            return -1;
        }

        strcpy(*(http_response->http_response_headers), HTTP_CONTENT_LENGTH_HEADER);
        *(http_response->http_response_headers) =
                strcat(strcat(*(http_response->http_response_headers), ": "), string_value);
    } else {

        char** aux_pointer = realloc(http_response->http_response_headers, (aux_header_index + 1) * sizeof(char*));
        if(aux_pointer == NULL) {

            return -1;
        }
        http_response->http_response_headers = aux_pointer;

        int size = strlen(HTTP_CONTENT_LENGTH_HEADER) + 2 + strlen(string_value); /** +2 for ": " */

        *(http_response->http_response_headers + aux_header_index) = malloc(size * sizeof(char));
        if(*(http_response->http_response_headers + aux_header_index) == NULL) {

            for(int i = 0; i < aux_header_index; i++) {

                free(*(http_response->http_response_headers + aux_header_index));
            }
            free(http_response->http_response_headers);
            return -1;
        }

        strcpy(*(http_response->http_response_headers + aux_header_index), HTTP_CONTENT_LENGTH_HEADER);
        *(http_response->http_response_headers + aux_header_index) =
                strcat(strcat(*(http_response->http_response_headers + aux_header_index), ": "), string_value);
    }

    aux_header_index++;
    *header_index = aux_header_index;
    return 0;
}

void clean_http_request_block(http_request_block_t* http_request) {

    if(http_request->http_request_headers != NULL) {

        free(http_request->http_request_headers);
    }
}

void clean_http_response_block(http_response_block_t* http_response, int* header_index) {

    if(http_response->http_response_headers != NULL) {

        for(int i = 0; i < *header_index; i++) {

            if(*(http_response->http_response_headers + i) != NULL) {

                free(*(http_response->http_response_headers + i));
            }
        }
        free(http_response->http_response_headers);
    }
    if(http_response->http_response_message_body != NULL) {

        free(http_response->http_response_message_body);
    }
}