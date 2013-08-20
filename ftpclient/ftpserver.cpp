#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>
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
                return BAD_SIZE;
            } else if (ferror(stream)) {
                d("Error encountered while reading file\n");
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

void process_connections(int listen_fd) {
    while (1) {
        struct sockaddr_in from;
        socklen_t fromSize = sizeof(from);
        pid_t pid = 0;
        int conn = srtp_accept(listen_fd, (struct sockaddr *) &from, &fromSize);
        if (conn == -1) {
            perror("Accept failed");
            break;
            continue;
        }
        if ((pid = fork()) == 0) {
            int result = copy_file(conn);
            if (result != 0) {
                fprintf(stderr, "Error encountered while transferring file\n");
            }
            d("Child terminating\n");
            close(conn);
            exit(result);
        } else {
            d("Accepted connection, child starting with PID %d\n", pid);
            close(conn);
        }
        
    }
}

void signal_handler(int sig) {
    int status;
    if (sig == SIGCHLD) {
        while (waitpid(-1, &status, WNOHANG) > 0) {
            if (WIFEXITED(status)) {
                d("Child reaped, exit status:%d\n", WEXITSTATUS(status));
            } else {
                d("Child reaped, child exited abnormally.. signal:%d\n",
                        WTERMSIG(status));
            }
        }
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
    /* be nice for reuse in case of testing */
    int opt = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
        perror("Error setting socket option");
        return 1;
    }
    
    /* bind */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                  //IPv4 || IPv6
    addr.sin_port = htons(port);                //port in network form
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
        debug = 1;
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
                "port in the range 1 to 65535\n", argv[2 + debug]);
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
    sigaction(SIGCHLD, &signals, NULL);
    
    /* start listening */
    process_connections(socket_fd);
    return 0;
}
