#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>
#include <pthread.h>
#include "../srtp/srtp.h"
#include "debug.h"

#define USAGE "USAGE: %s [-d] port\n"
#define BAD_ARGS 1
#define BAD_PORT 2
#define RUNTIME_ERROR 3
#define BAD_REQUEST 4
#define FILE_EXISTS 5
#define CANNOT_CREATE 6
#define BAD_SIZE 7

extern int debug;

int socket_fd; /* for use in the SIGINT handler */

int copy_file(int in) {
    FILE *stream = fdopen(in, "r");
    char buffer[512] = {'\0'}; //WRQ|filename\nsize\n\0
    long filesize = -1, read = 0;
    /* read the request type */
    if (fgets(buffer, 512, stream) == NULL || strlen(buffer) < 6 
            || strncmp(buffer, "WRQ|", 4) != 0) {
        d("Request was not of valid format or "
                "filename was the empty string\n");
        return BAD_REQUEST;
    }
    char *temp = strchr(buffer, '\n');
    /* newline is missing or newline immediately follows | */
    if (temp == NULL) {
        d("Request was not of valid format\n");
        return BAD_REQUEST;
    }
    /* remove the \n so we can use it in our file ops */
    *temp = '\0';
    /* strip any leading directories (stop "/etc/passwd" etc) */ 
    char *filename = strrchr(buffer + 4, '/');
    if (filename == NULL) {
        filename = buffer + 4;
    } else {
        filename += 1; /* moves it past the '/' char */
    }
    d("Valid request recieved: '%s'\n", buffer);
    /* check file doesn't already exist */
    if (access(filename, F_OK) != -1) {
        d("Given file '%s' already exists\n", buffer + 4);
        return FILE_EXISTS;
    }
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return CANNOT_CREATE;
    }
    char dummy = '\0';
    if (fgets(buffer, 512, stream) == NULL 
            || sscanf(buffer, "%ld%c", &filesize, &dummy) != 2 
            || filesize < 0 || dummy != '\n') {
        d("Given size missing or invalid\n");
        return BAD_REQUEST;
    }
    
    /* read enough bytes */
    while (read < filesize) {
        int chunksize = (filesize - read > 512) ? 512 : filesize - read;
        int actual = fread(buffer, sizeof(char), chunksize, stream);
        read += actual;
        if (actual < chunksize) {
            if (feof(stream)) {
                d("Unexpected end of file reached\n");
                fclose(file);
                unlink(filename);
                return BAD_SIZE;
            } else if (ferror(stream)) {
                d("Error encountered while reading file\n");
                fclose(file);
                unlink(filename);
                return RUNTIME_ERROR;
            }
        } else {
            int written = fwrite(buffer, sizeof(char), actual, file);
            if (written != actual) {
                perror("Local write");
                d("Writing to local file failed\n");
                return RUNTIME_ERROR;
            }
        }
    }
    if (fgetc(stream) != EOF) {
        d("Extra data recieved\n");
        return BAD_SIZE;
    }
    d("File transfer complete\n");
    fclose(file);
    return 0;
}

void *client_thread(void *arg) {
    int fd = *((int *)arg);
    free(arg);
    int state = copy_file(fd);
    d("Child terminating\n");
    srtp_close(fd, state);
    return NULL;
}

void process_connections(int listen_fd) {
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        fprintf(stderr, "pthread_attr_init() failed. "
                "This function should never fail.");
    }
    socket_fd = listen_fd;
    while (1) {
        struct sockaddr_in from;
        socklen_t fromSize = sizeof(from);
        d("Waiting for connection\n", listen_fd);
        int *conn = (int *) malloc(sizeof(int));
        *conn = srtp_accept(listen_fd, (struct sockaddr *) &from, &fromSize);
        if (*conn == -1) {
            perror("Accept failed");
            break;
            continue;
        }
        pthread_t thread;
        if (pthread_create(&thread, &attr, &client_thread, (void *) conn)) {
            perror("Client thread creation failed:");
            close(*conn);
            free(conn);
            continue;
        }
        pthread_detach(thread);
        d("Accepted connection, thread started\n");
    }
}

/* this is just here to stop SIGPIPE crashing the server */
void signal_handler(int sig) {
    if (sig == SIGINT) {
        srtp_close(socket_fd, 0);
        exit(0);
    }
}

int setup_socket(int *sock, int port) {
    /* create socket */
    *sock = srtp_socket(AF_INET, SOCK_STREAM, 0); /* 0 == any protocol */
    if (*sock == -1) {
        perror("Error opening socket");
        return 1;
    }
    d("Socket fd: %d\n", *sock);
    /* bind */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                  //IPv4
    addr.sin_port = port;//htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    //any local IP is good
    if (srtp_bind(*sock, (struct sockaddr*) &addr,
            sizeof(struct sockaddr_in)) == -1) {
        perror("Error while binding socket");
        return 1;
    }
    
    /* listen */
    if (srtp_listen(*sock, SOMAXCONN) == -1) {
        perror("Error starting listen");
        return 1;
    }
    
    return 0;
}

int main(int argc, char **argv){
    unsigned int port = 0;

    if (argc == 3) {
        /* -d not supplied */
        if (strncmp("-d", argv[1], 2) != 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
        srtp_debug(debug = 1);
    } else if (argc == 2) {
        /* -d supplied */
        if (strncmp("-d", argv[1], 2) == 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
    } else {
        fprintf(stderr, USAGE, argv[0]);
        return BAD_ARGS;
    }

    /* check the port number */
    char crap;
    if ((sscanf(argv[1 + debug], "%u%c", &port, &crap)) != 1 || port < 1 
            || port > 65535) {
        fprintf(stderr, "The port '%s' is invalid, please provide a "
                "port in the range 1 to 65535\n", argv[1 + debug]);
        return BAD_PORT;
    }
    d("Port valid: %d\n", port);
    
    /* set up the socket */
    int socket_fd = 0;
    if (setup_socket(&socket_fd, port) != 0) {
        return RUNTIME_ERROR;
    }
    d("Socket created successfully\n");
    /* register signal handlers before we get carried away*/
    struct sigaction signals;
    signals.sa_handler = signal_handler;
    signals.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &signals, NULL);
    sigaction(SIGINT, &signals, NULL);
    
    /* start listening */
    process_connections(socket_fd);
    return 0;
}
