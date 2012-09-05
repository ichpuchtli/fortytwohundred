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

#define PACKET_SIZE 512
#define HEADER_SIZE 2
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

enum PKTDIR {
  SEND, RECV
};

int debug = 0;

#define d2(...) fprintf(stderr,__VA_ARGS__)

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

void print_packet(struct sockaddr_in* addr_local, struct sockaddr_in* addr_remote, char* pktbuffer, enum PKTDIR dir) {
    //uint16_t port_local=0;
    //uint16_t port_remote=0;
    //char str_local[16];
    //char str_remote[16];
    char cmdstr[16];
    char timestr[16];
    char arrow='<';
    //uint8_t seq=0;
    uint8_t len=0;

    //strcpy(str_local, "xxx.xxx.xxx"); 
    //strcpy(str_remote, "xxx.xxx.xxx");
    strcpy(cmdstr, "ACK");
    strcpy(timestr, "hh:mm:ss");

    arrow = (dir==SEND?'<':'>');
    //port_local = ntohs(addr_local->sin_port);
    //port_remote = ntohs(addr_remote->sin_port);
    //str_remote=inet_ntoa(addr_remote->sin_addr.s_addr);
    //str_local=inet_ntoa(addr_local->sin_addr.s_addr);

    d2("[%u] %s | %s:%u %c %s %c %s:%u | seq=%u len=%u\n", 
        getpid(), timestr, inet_ntoa(addr_remote->sin_addr), 
        ntohs(addr_remote->sin_port), arrow, 
        cmdstr, arrow, inet_ntoa(addr_local->sin_addr), 
        ntohs(addr_local->sin_port), pktbuffer[1], len);
}

#endif
