#ifndef SRTP_COMMON_H
#define SRTP_COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#include "list.h"

/*
 * Define the commands here
 */
#define CMD_NONE ((uint8_t)0)
#define CMD_ACK ((uint8_t)1)
#define CMD_BADREQ ((uint8_t)2)
#define CMD_EXISTS ((uint8_t)3)
#define CMD_DATA ((uint8_t)4)
#define CMD_WRQ ((uint8_t)5)
#define CMD_FIN ((uint8_t)6)
//...
#define MAX_CMD_VALUE ((uint8_t)6)

const char* command_strings[(size_t)(MAX_CMD_VALUE+1)]={
    "", "ACK", "BRQ", "FEX", "DAT", "WRQ", "FIN"
};

#define PACKET_SIZE 512
#define HEADER_SIZE 4
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

#define WINDOW_SIZE 50

enum PKTDIR {
  SEND, RECV
};

#define GETLEN(buf) ntohs(*((uint16_t*)(&buf[2])))
#define SETLEN(buf, len) (*((uint16_t*)(&buf[2])) = htons(len))

#define d2(...) fprintf(stderr,__VA_ARGS__)

int debug = 0;

void _debug(const char *format, va_list args) {
    fprintf(stderr, "Debug: ");
    vfprintf(stderr, format, args);
}

void d(const char *format, ...) {
    if (debug) {
        va_list args;
        va_start(args, format);
        _debug(format, args);
        va_end(args);
    }
}

char* command2str(uint8_t command) {
    static char s[16];

    if (command>MAX_CMD_VALUE) {
        sprintf(s,"?%u?",command);
        return (char*)s;
    } else return (char*)command_strings[command];
}

void print_packet(struct sockaddr_in* addr_local,
        struct sockaddr_in* addr_remote, char* pktbuffer, enum PKTDIR dir) {
    char timestr[32]={'\0'};
    char str_remote[32];
    char str_local[32];
    char arrow='<';
    time_t rawtime;
    struct tm * timeinfo;

    arrow = (dir==SEND?'<':'>');

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestr,32,"%H:%M:%S",timeinfo);

    strncpy(str_remote,inet_ntoa(addr_remote->sin_addr),32);
    strncpy(str_local,inet_ntoa(addr_local->sin_addr),32);

    d2("[%u] %s | %s:%u %c %s %c %s:%u | seq=%u len=%u\n", 
        getpid(), timestr, str_remote, ntohs(addr_remote->sin_port), arrow, 
        command2str(pktbuffer[0]), arrow, str_local, 
        ntohs(addr_local->sin_port), pktbuffer[1], GETLEN(pktbuffer) 
        /*ntohs((uint16_t)(pktbuffer[2]))i*/);
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

struct EndPoint {
    union {
        struct sockaddr *base;
        struct sockaddr_in *in;
    } addr;
    socklen_t len;
};

int send_packet(int sock, struct EndPoint *target, char *packet, 
        struct sockaddr *local) {
    print_packet((struct sockaddr_in *) local,
            (struct sockaddr_in *) target->addr.in, packet, SEND);

    size_t packetSize = HEADER_SIZE + GETLEN(packet);

    return sendto(sock, packet, packetSize, 0, target->addr.base,
            target->len) != packetSize;
}

int send_all(int sock, struct EndPoint *target, struct List *packets, 
        struct sockaddr *local) {
    if (packets->head == NULL) {
        return 0;
    }
    for (struct ListNode *n = packets->head; n != NULL; n = n->next) {
        if (send_packet(sock, target, (char *) n->data, local)) {
            return 1;
        }
    }
    return 0;
}

char *create_packet(uint8_t command, uint8_t sequence, uint16_t payloadSize,
        char *payload) {
    char *buffer = malloc(sizeof(char) * (payloadSize + HEADER_SIZE));
    buffer[0] = command;
    buffer[1] = sequence;
    SETLEN(buffer, payloadSize);
    if (payload != NULL) {
        memcpy(buffer + HEADER_SIZE, payload, payloadSize);
    }
    return buffer;
} 

ssize_t read_only_from(int sock, char *buffer, size_t packetSize, int flags,
        struct sockaddr *wanted, socklen_t *wantedSize) {
    ssize_t read = 0;
    char *temp = malloc(sizeof(char) * packetSize);
    if (temp == NULL) {
        return -1;
    }
    struct sockaddr from;
    socklen_t fromSize;
    read = recvfrom(sock, temp, packetSize, flags, &from, &fromSize);
    // is this from the endpoint we care about?
    if (1 || read > 0 && !memcmp(from.sa_data, wanted->sa_data, 14)) {
        memcpy(buffer, temp, packetSize);
        free(temp);
        return read;
    }
    d("Discarding foreign packet\n");
    //no? well, ignore it
    free(temp);
    errno = EAGAIN;
    return -1;
}

ssize_t read_until_timeout(int socket, char *buffer, size_t packetSize, int flags,
        struct EndPoint *end) {
    time_t startTime = time(NULL);
    ssize_t read;
    while (time(NULL) - startTime < 6) {
        read = read_only_from(socket, buffer, PACKET_SIZE, MSG_DONTWAIT,
                end->addr.base, &end->len);
        if (read > 0) {
            return read;
        }
        usleep(10000);
    }
    d("Connection timed out\n");
    return -1;
}

#endif
