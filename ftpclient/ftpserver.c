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
#define TIMEOUT 8

extern int debug, errno;

/* both for use in the SIGINT handler */
struct List *children = NULL;
int socket_fd;

int copy_file(char *buffer, struct EndPoint *origin) {
    int socket = 0;

    char *payload = buffer + HEADER_SIZE;

    char *packet = NULL;

    struct sockaddr addr;
    socklen_t mySize = sizeof(struct sockaddr); 
    if (setup_socket(&socket, 0) != 0) {
        return RUNTIME_ERROR;
    }
    getsockname(socket, &addr, &mySize);
    d("Child process opening ephemeral port %d\n", 
            ntohs(((struct sockaddr_in *)&addr)->sin_port));
    /* read the request type */
    if (buffer[0] != CMD_WRQ && buffer[1] != 0) {
        d("Request was not of valid format\n");
        packet = create_packet(CMD_BADREQ, 0, 0, NULL);
        if (send_packet(socket, origin, packet, &addr)) {
            perror("Error denying request");
            exit(RUNTIME_ERROR);
        }
        //free(packet);
        return BAD_REQUEST;
    }
    /* strip any leading directories (stop "/etc/passwd" etc) */ 
    char *filename = strrchr(payload, '/');
    char *separator = strchr(payload, '|');

    if (separator == NULL) {
        d("Missing file size\n");
        packet = create_packet(CMD_BADREQ, 0, 0, NULL);
        if (send_packet(socket, origin, packet, &addr)) {
            perror("Error denying request");
            exit(RUNTIME_ERROR);
        }
        free(packet);
        return BAD_REQUEST;
    } else {
        *separator = '\0';
    }
    if (filename == NULL) {
        filename = malloc(sizeof(char) * (strlen(payload) + 1));
        strcpy(filename, payload);
    } else {
        char *t = malloc(sizeof(char) * (strlen(filename + 1) + 1));
        strncpy(t, filename + 1, strlen(filename) + 1);
        filename = t;
    }

    d("Valid request recieved: '%s'\n", filename);
    *separator = '\0';
    /* check file doesn't already exist */
    if (access(filename, F_OK) != -1) {
        d("Given file '%s' already exists\n", filename);
        packet = create_packet(CMD_EXISTS, 0, 0, NULL);
        if (send_packet(socket, origin, packet, &addr)) {
            perror("Error denying request");
            exit(RUNTIME_ERROR);
        }
        //free(packet);
        return FILE_EXISTS;
    }
    char dummy = '\0';
    long filesize = -1;
    if (sscanf(separator + 1, "%ld%c", &filesize, &dummy) != 1 
            || filesize < 0) {
        d("Given size invalid\n");
        return BAD_REQUEST;
    }
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        packet = create_packet(CMD_EXISTS, 0, 0, NULL);
        if (send_packet(socket, origin, packet, &addr)) {
            perror("Error denying request");
            exit(RUNTIME_ERROR);
        }
        return CANNOT_CREATE;
    }
    // send an ACK from our new ephemeral port
    struct sockaddr_in from;
    socklen_t fromSize = sizeof(struct sockaddr_in);
    int size_read = -1;
    time_t startTime = time(NULL);
    packet = create_packet(CMD_ACK, 1, 0, payload);
    do {
        if (send_packet(socket, origin, packet, &addr)) {
            perror("Error acking request");
            *separator = '\0';
            unlink(filename);
            exit(RUNTIME_ERROR);
        }
        sleep(1);
    } while (size_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)
            && time(NULL) - startTime < 6);
    //free(packet);
    packet = NULL;

    long read = 0;
    uint8_t expected_sequence = 1;
    /* read enough bytes */
    while (read < filesize) {
        int chunksize = (filesize - read > PAYLOAD_SIZE) ? 
                PAYLOAD_SIZE : filesize - read;
        int actual = read_until_timeout(socket, buffer, PACKET_SIZE,
                MSG_DONTWAIT, origin);
        if (actual == -1) {
            d("deleting partial file: %s\n", filename);
            unlink(filename);
            exit(TIMEOUT);
        } else if (buffer[0] != CMD_DATA) {
            d("Received command %s\n", command2str(buffer[0]));
        } else {
            if ((uint8_t) buffer[1] != expected_sequence) {
                packet = create_packet(CMD_ACK, expected_sequence, 0, NULL);
                send_packet(socket, origin, packet, &addr);
                //free(packet);
                continue;
            }
            actual = ntohs(*(buffer + 2));
            read += actual;
            int written = fwrite(payload, sizeof(char), actual, file);
            if (written != actual) {
                perror("Local write");
                d("Writing to local file failed\n");
                return RUNTIME_ERROR;
            }
        }
    }
    packet = create_packet(CMD_FIN, 1, 0, NULL);
    send_packet(socket, origin, packet, &addr);
    d("File transfer complete\n");
    close(socket);
    fclose(file);
    return 0;
}

void kill_children(void) {
    for (struct ListNode *n = children->head; n != NULL; n = n->next) {
        kill(*((pid_t *) n->data), SIGINT);
        free(n->data);
    }
}

void process_connections(int listen_fd) {
    socket_fd = listen_fd;
    char buffer[PACKET_SIZE];
    struct sockaddr_in from;
    struct sockaddr mine;
    socklen_t myLength;
    getsockname(listen_fd, &mine, &myLength);
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
        //buffer[2] = (uint16_t) GETLEN(buffer);

        print_packet((struct sockaddr_in *)&mine, &from, buffer, RECV);
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
                struct EndPoint origin = {.addr.in = &from, .len = fromSize};
                exit(copy_file(buffer, &origin));
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
        if (errno != ECHILD && pid < 0) {
            perror("Error attempting to reap child");
        }
    }
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
