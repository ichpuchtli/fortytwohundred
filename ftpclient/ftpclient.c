#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "srtp_common.h"

#define USAGE "USAGE: %s [-d] server port filename\n"
#define BAD_ARGS 1
#define BAD_PORT 2
#define BAD_FILE 3
#define RUNTIME_ERROR 4
#define TIMEOUT 8

extern int debug, errno;

void signal_handler(int signal) {
    fprintf(stderr, "SIGPIPE received, exiting\n");
    exit(RUNTIME_ERROR);
}

int copy_file(FILE *file, int sock, char *filename, struct sockaddr *addr,
        socklen_t *addrLen) {
    FILE *stream = fdopen(sock, "wb");
    if (stream == NULL) {
        perror("fdopen(socket_fd)");
        return RUNTIME_ERROR;
    }
    char buffer[512] = {0};
    struct stat s;
    stat(filename, &s);
    long size = s.st_size;
    d("Sending request\n");
    time_t startTime = time(NULL);
    int size_read = 0;
    do {
        fprintf(stream, "WRQ|%s|%ld", filename, size);
        fflush(stream);
        sleep(1);
        size_read = recvfrom(sock, buffer, PACKET_SIZE, MSG_DONTWAIT,
                addr, addrLen);
    } while (size_read < 0 && errno == EWOULDBLOCK && time(NULL) - startTime < 6);
    if (size_read < 0) {
        fprintf(stderr, "Connection to server timed out.\n");
        exit(TIMEOUT);
    }
    
    long written = 0;
    while (written < size && !feof(file) && !ferror(file)) {
        size_t chunksize = fread(buffer, sizeof(char), 512, file);
        if (fwrite(buffer, sizeof(char), chunksize, stream) != chunksize) {
            d("Error writing to network stream\n");
            return RUNTIME_ERROR;
        }
    }
    fflush(stream);
    d("File transfer complete\n");
    return 0;
}

void process_address(struct addrinfo **addr, char *hostname, char *port) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    
    int ai = -1;
    if ((ai = getaddrinfo(hostname, port, &hints, addr)) != 0) {
        fprintf(stderr, "Error getting address info: %s\n", gai_strerror(ai));
        exit(BAD_PORT);
    }
}

int main(int argc, char **argv){
    unsigned int port = 0;
    FILE *file = NULL;
    if (argc == 5) {
        /* -d not supplied */
        if (strncmp("-d", argv[1], 2) != 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
        debug = 1;
    } else if (argc == 4) {
        /* -d supplied */
        if (strncmp("-d", argv[1], 2) == 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
    } else {
        fprintf(stderr, USAGE, argv[0]);
        return BAD_ARGS;
    }
    /* check the port */
    char crap;
    if ((sscanf(argv[2 + debug], "%u%c", &port, &crap)) != 1 || port < 1 
            || port > 65535) {
        fprintf(stderr, "The port '%s' is invalid, please provide a "
                "port in the range 1 to 65535\n", argv[2 + debug]);
        return BAD_PORT;
    }
    d("Port OK: %d\n", port);
    /* check the file */
    if ((file = fopen(argv[3 + debug], "rb")) == NULL) {
        perror("Error opening given file");
        return BAD_FILE;
    }
    d("File opened successfully\n");
    /* set up the signal handler */
    struct sigaction signals;
    signals.sa_handler = signal_handler;
    signals.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &signals, NULL);
    
    /* check address, set up socket, connect */
    struct addrinfo *addr = NULL;
    process_address(&addr, argv[1 + debug], argv[2 + debug]);
    d("getaddrinfo() successful\n");
    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == -1) {
        perror("Error opening socket");
        exit(RUNTIME_ERROR);
    }
    d("Socket opened successfully, fd: %d\n", sock);
    if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1) {
        perror("Error connecting to server");
        close(sock);
        exit(RUNTIME_ERROR);
    }
    freeaddrinfo(addr);
    d("Connected successfully\n");
    /* request to write (reads not required in the assignment) */
    copy_file(file, sock, argv[3 + debug], addr->ai_addr, &(addr->ai_addrlen));
    d("Exiting without errors\n");
    fclose(file);
    close(sock);
    return 0;
}
