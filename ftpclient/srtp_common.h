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
//...
#define MAX_CMD_VALUE ((uint8_t)1)

const char* command_strings[(size_t)(MAX_CMD_VALUE+1)]={
    "", "ACK"
};

#define PACKET_SIZE 512
#define HEADER_SIZE 2
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

enum PKTDIR {
  SEND, RECV
};

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

void print_packet(struct sockaddr_in* addr_local, struct sockaddr_in* addr_remote, char* pktbuffer, enum PKTDIR dir) {
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
        ntohs(addr_local->sin_port), pktbuffer[1], 
        ntohs((uint16_t)(pktbuffer[2])));
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


#endif
