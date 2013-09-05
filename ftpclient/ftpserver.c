#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "srtp_common.h"

#define USAGE "USAGE: %s [-d] port\n"
#define BAD_ARGS 1
#define BAD_PORT 2
#define RUNTIME_ERROR 3
#define BAD_REQUEST 4
#define FILE_EXISTS 5
#define CANNOT_CREATE 6
#define BAD_SIZE 7

extern int debug, errno;

struct List *children = NULL;

int setup_socket(int *, int); /* socket, portnum (0 = ANY) */

int socket_fd; /* for use in the SIGINT handler */

int copy_file(char *buffer, struct sockaddr_in origin, 
        size_t originLength) {
    int socket = 0;
    struct sockaddr addr;
    socklen_t mySize = sizeof(struct sockaddr); 
    if (setup_socket(&socket, 0) != 0) {
        return RUNTIME_ERROR;
    }
    getsockname(socket, &addr, &mySize);
    d("Child process opening emphemeral port %d\n", 
            ntohs(((struct sockaddr_in *)&addr)->sin_port));
    long filesize = -1, read = 0;
    print_packet((struct sockaddr_in *)&addr, &origin, buffer, RECV);
    /* read the request type */
    if (strncmp(buffer, "WRQ|", 4) != 0) {
        d("Request was not of valid format or "
                "filename was the empty string\n");

        return BAD_REQUEST;
    }
    char *temp = strchr(buffer, '\0');
    /* newline is missing or newline immediately follows | */
    if (temp == NULL) {
        d("Request was not of valid format\n");
        return BAD_REQUEST;
    }
    /* remove the \n so we can use it in our file ops */
    *temp = '\0';
    /* strip any leading directories (stop "/etc/passwd" etc) */ 
    char *filename = strrchr(buffer + 4, '/');
    char *separator = strrchr(buffer, '|');
    if (filename == NULL) {
        filename = buffer + 4;
    } else {
        filename += 1; /* moves it past the '/' char */
    }
    if (separator == NULL) {
        d("Missing file size\n");
        return BAD_REQUEST;
    }
    d("Valid request recieved: '%s'\n", buffer);
    *separator = '\0';
    /* check file doesn't already exist */
    if (access(filename, F_OK) != -1) {
        d("Given file '%s' already exists\n", buffer + 4);
        
        return FILE_EXISTS;
    }
    char dummy = '\0';
    if (sscanf(separator + 1, "%ld%c", &filesize, &dummy) != 1 
            || filesize < 0) {
        d("Given size invalid\n");
        return BAD_REQUEST;
    }
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return CANNOT_CREATE;
    }
    // send an ACK from our new ephemeral port
    buffer[0] = 'A', buffer[1] = 'C', buffer[2] = 'K', *separator = '|';
    if (sendto(socket, buffer, PACKET_SIZE, 0, (struct sockaddr *) &origin,
            originLength) != PACKET_SIZE) {
        perror("Error acking request\n");
        *separator = '\0';
        unlink(filename);
        exit(RUNTIME_ERROR);
    }
    
    struct sockaddr_in from;
    socklen_t fromSize = sizeof(struct sockaddr_in);
    
    /* read enough bytes */
    while (read < filesize) {
        int chunksize = (filesize - read > PAYLOAD_SIZE) ? 
                PAYLOAD_SIZE : filesize - read;
        int actual = recvfrom(socket, buffer, PACKET_SIZE, 0,
                (struct sockaddr *) &from, &fromSize);
        read += actual;
        if (actual < chunksize) {
            if (actual = 0) {
                fprintf(stderr, "TODO:  ");
                d("Read nothing from that recvfrom\n");
                return BAD_SIZE;
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
    d("File transfer complete\n");
    close(socket);
    fclose(file);
    return 0;
}

void kill_children(void) {
    for (struct ListNode *n = children->head; n != NULL; n = n->next) {
        kill(*((pid_t *) n->data), SIGINT);
    }
}

void process_connections(int listen_fd) {
    socket_fd = listen_fd;
    char *buffer = malloc(sizeof(char) * PACKET_SIZE);
    struct sockaddr_in from;
    socklen_t fromSize = sizeof(from);
    children = malloc(sizeof(struct List));
    if (children == NULL) {
        fprintf(stderr, "malloc() failed, exiting\n");
        close(listen_fd);
        exit(RUNTIME_ERROR);
    }
    init_list(children);
    while (1) {
        memset(buffer, 0, PACKET_SIZE);
        d("Waiting for connection\n", listen_fd);
        int n = recvfrom(listen_fd, buffer, PACKET_SIZE, 0,
                (struct sockaddr *) &from, &fromSize);
        if (n == 0) {
            d("Empty message received\n");
            continue;
        }
        pid_t *pid = malloc(sizeof(pid_t));
        *pid = fork();
        switch (*pid) {
            case -1:
                perror("Fork failed");
                d("Unable to service request, ignoring\n");
                free(pid);
                break;
            case 0:
                free(pid);
                exit(copy_file(buffer, from, fromSize));
                break;
            default:
                d("Recieved request, forked off responder (PID %d)\n", *pid);
                if (add_to_list(children, (void *) pid) != 0) {
                    kill_children();
                    exit(RUNTIME_ERROR);
                }
                break;
        }
    }
}

/* this is just here to stop SIGPIPE crashing the server */
void signal_handler(int sig) {
    if (sig == SIGINT) {
        close(socket_fd);
        kill_children();
        exit(0);
    } else if (sig == SIGCHLD) {
        int status;
        pid_t pid = 1;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (WIFSIGNALED(status)) {
                fprintf(stderr, "Child %d died to signal %d\n", pid, 
                        WTERMSIG(status));
            } else if (WIFEXITED(status)) {
                status = WEXITSTATUS(status);
                if (status != 0) {
                    d("Child %d exited with status %d\n", pid, status);
                } else {
                    d("Child %d exited normally\n", pid);
                }
            }
        }
        if (errno != ECHILD) {
            perror("Error attempting to reap child");
        }
    }
}

int setup_socket(int *sock, int port) {
    /* create socket */
    *sock = socket(AF_INET, SOCK_DGRAM, 0); /* 0 == any protocol */
    if (*sock == -1) {
        perror("Error opening socket");
        return 1;
    }
    d("Socket fd: %d\n", *sock);
    /* bind */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                  //IPv4
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    //any local IP is good
    if (bind(*sock, (struct sockaddr*) &addr,
            sizeof(struct sockaddr_in)) == -1) {
        perror("Error while binding socket");
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
    sigaction(SIGCHLD, &signals, NULL);
    sigaction(SIGINT, &signals, NULL);
    
    /* start listening */
    process_connections(socket_fd);
    return 0;
}
