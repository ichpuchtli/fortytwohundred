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
#define RUNTIME_ERROR 3
#define BAD_REQUEST 4
#define FILE_EXISTS 5
#define BAD_FILE 6
#define TIMEOUT 8

extern int debug, errno;

void signal_handler(int signal) {
    fprintf(stderr, "SIGPIPE received, exiting\n");
    exit(RUNTIME_ERROR);
}

int copy_file(FILE *file, int sock, char *filename, struct sockaddr *addr,
        socklen_t *addrLen) {
    char buffer[512] = {0};
    struct stat s;
    struct sockaddr mine;
    socklen_t mySize;
    if (getsockname(sock, &mine, &mySize) == -1) {
        perror("getting sock name:");
    }
    stat(filename, &s);
    long size = s.st_size;
    d("Sending request\n");
    time_t startTime = time(NULL);
    int size_read = 0;
    sprintf(buffer, "WRQ|%s|%ld", filename, size);
    do {
        print_packet((struct sockaddr_in *)addr, (struct sockaddr_in *) &mine, 
                buffer, SEND);
        if (sendto(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *) addr,
                *addrLen) != PACKET_SIZE) {
            perror("Error sending request");
            exit(RUNTIME_ERROR);
        }
        sleep(1);
        size_read = recvfrom(sock, buffer, PACKET_SIZE, MSG_DONTWAIT,
                addr, addrLen);
    } while (size_read < 0 && (errno == EWOULDBLOCK || errno == EAGAIN) 
            && time(NULL) - startTime < 6);
    if (size_read < 0) {
        fprintf(stderr, "Connection to server timed out.\n");
        exit(TIMEOUT);
    }
    if (buffer[0] == 'A') {
        d("Request accepted\n");
    } else if (buffer[0] == FILE_EXISTS) {
        printf("%s already exists on the server, server rejected\n", filename);
        exit(FILE_EXISTS);
    }
    long written = 0;
    uint8_t sequence = 1;
    while (written < size && !feof(file) && !ferror(file)) {
        size_t chunksize = fread(buffer + HEADER_SIZE, sizeof(char),
                PAYLOAD_SIZE, file);
        buffer[0] = 'D';
        buffer[1] = sequence++;
        *(buffer + 2) = htons(chunksize);
        print_packet((struct sockaddr_in *)addr, (struct sockaddr_in *) &mine, 
                buffer, SEND);
        if (sendto(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *) addr,
                *addrLen) != PACKET_SIZE) {
            perror("Error sending data packet");
            exit(RUNTIME_ERROR);
        }
        //if (fwrite(buffer, sizeof(char), chunksize, stream) != chunksize) {
        //    d("Error writing to network stream\n");
        //    return RUNTIME_ERROR;
        //}
    }
    d("File transfer complete\n");
    return 0;
}

int process_address(struct sockaddr **addr, socklen_t *len, 
        char *hostname, char *port) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    int ai = -1;
    if ((ai = getaddrinfo(hostname, port, &hints, &result)) != 0) {
        fprintf(stderr, "%s:%d\n", hostname, port);
        fprintf(stderr, "Error getting address info: %s\n", gai_strerror(ai));
        exit(BAD_PORT);
    }
    *addr = result->ai_addr;
    *len = result->ai_addrlen;
    return 0;
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
    struct sockaddr *addr;
    socklen_t addrLen;
    process_address(&addr, &addrLen, argv[1 + debug], argv[2 + debug]);
    d("getaddrinfo() successful\n");
    int sock;
    if (setup_socket(&sock, 0) != 0) {
        return RUNTIME_ERROR;
    }
    d("Bound successfully\n");
    /* request to write (reads not required in the assignment) */
    copy_file(file, sock, argv[3 + debug], addr, &addrLen);
    d("Exiting without errors\n");
    fclose(file);
    close(sock);
    return 0;
}
