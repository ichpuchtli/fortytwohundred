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
#define CMD_NONE ((uint8_t)0) ///@brief No command packet
#define CMD_ACK ((uint8_t)1) ///@brief Acknowledgement command
#define CMD_BADREQ ((uint8_t)2) ///@brief Bad Request command
#define CMD_EXISTS ((uint8_t)3) ///@brief File already exists command
#define CMD_DATA ((uint8_t)4) ///@brief Data packet command
#define CMD_WRQ ((uint8_t)5) ///@brief Write Request command
#define CMD_FIN ((uint8_t)6) ///@brief Finish / connection termination command
#define CMD_FINACK ((uint8_t)7) ///@brief Finish Acknowledgement

#define MAX_CMD_VALUE ((uint8_t)7)

const char* command_strings[( size_t )( MAX_CMD_VALUE + 1 )] = {
  "", "ACK", "BRQ", "FEX", "DAT", "WRQ", "FIN", "F+A"
};

/**
 * @brief Maximum packet size for SRTP packets
 */
#define PACKET_SIZE 1024

/**
 * @brief The SRTP header size
 */
#define HEADER_SIZE 5

/**
 * @brief The maximum SRTP packet payload size
 */
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

/**
 * @brief The size of the sliding window monitoring the input file
 */
#define WINDOW_SIZE 50

/**
 * @brief The Acknowledgement timeout in seconds
 */
#define ACK_TIMEOUT 2

enum PKTDIR {
  SEND,
  RECV
};

/**
 * @brief get the packet len specified in the SRTP header in host order form
 *
 * @param buffer a pointer to the SRTP packet
 *
 * @return a 16bit integer specifying the length of the packet
 */
uint16_t GETLEN( char* buffer )
{

  uint16_t* len = ( uint16_t* )( buffer + 3 );

  return  ntohs( *len );

}

/**
 * @brief set the packet length of the SRTP packet in network order form
 *
 * @param buffer a pointer to the start of the SRTP packet
 * @param len the length of packet
 */
void SETLEN( char* buffer, uint16_t len )
{

  uint16_t* len_ptr = ( uint16_t* )( buffer + 3 );

  *len_ptr = htons( len );

}

#define d2(...) fprintf(stderr,__VA_ARGS__)

/**
 * @brief the global debug switch variable
 */
int debug = 0;

/**
 * @brief debug print helper function
 *
 * @param format text format
 * @param args variable arguments
 */
void _debug( const char *format, va_list args )
{
  fprintf( stderr, "Debug: " );
  vfprintf( stderr, format, args );
}

/**
 * @brief another debug print helper function
 *
 * @param format text format
 * @param ... variable arguments
 */
void d( const char *format, ... )
{
  if ( debug ) {
    va_list args;
    va_start( args, format );
    _debug( format, args );
    va_end( args );
  }
}

/**
 * @brief get the sequence number of a SRTP packet in host order form
 *
 * @param buffer the SRTP packet buffer
 *
 * @return the sequence number in host order form
 */
uint16_t get_seq_num( char* buffer )
{

  uint16_t* seq = ( uint16_t* )( buffer + 1 );

  return  ntohs( *seq );

}

/**
 * @brief set the sequence number for an SRTP packet in network order form
 *
 * @param buffer the buffer containing the soon to be SRTP packet
 * @param seq_num the sequence number to set
 */
void set_seq_num( char* buffer, uint16_t seq_num )
{

  uint16_t* seq = ( uint16_t* )( buffer + 1 );

  *seq = htons( seq_num );

}

/**
 * @brief produce a human readable version of a command
 *
 * @param command the command to print
 *
 * @return a string representation of the command
 */
char* command2str( uint8_t command )
{
  static char s[16];

  if ( command > MAX_CMD_VALUE ) {
    sprintf( s, "?%u?", command );
    return ( char* )s;
  } else return ( char* )command_strings[command];
}

/**
 * @brief print a packet in a human readable form
 *
 * @param addr_local the local address
 * @param addr_remote the remote address
 * @param pktbuffer the packet buffer
 * @param dir the message direction
 */
void print_packet( struct sockaddr_in* addr_local,
                   struct sockaddr_in* addr_remote, char* pktbuffer, enum PKTDIR dir )
{

  if ( debug ) {

    char timestr[32] = {'\0'};
    char str_remote[32];
    char str_local[32];
    char arrow = '<';
    time_t rawtime;
    struct tm * timeinfo;

    arrow = ( dir == SEND ? '<' : '>' );

    time( &rawtime );
    timeinfo = localtime( &rawtime );
    strftime( timestr, 32, "%H:%M:%S", timeinfo );

    strncpy( str_remote, inet_ntoa( addr_remote->sin_addr ), 32 );
    strncpy( str_local, inet_ntoa( addr_local->sin_addr ), 32 );

    d2( "[%u] %s | %s:%u %c %s %c %s:%u | seq=%u len=%u\n",
        getpid(), timestr, str_remote, ntohs( addr_remote->sin_port ), arrow,
        command2str( pktbuffer[0] ), arrow, str_local,
        ntohs( addr_local->sin_port ), get_seq_num( pktbuffer ), GETLEN( pktbuffer )
        /*ntohs((uint16_t)(pktbuffer[2]))i*/ );
  }
}

/**
 * @brief create a socket with familiar settings
 *
 * @param sock a pointer to an int which will contain the new UDP socket fd
 * @param port the port to bind the socket to
 *
 * @return 0 on success, non-zero otherwise
 */
int setup_socket( int *sock, int port )
{
  /* create socket */
  *sock = socket( AF_INET, SOCK_DGRAM, 0 ); /* 0 == any protocol */
  if ( *sock == -1 ) {
    perror( "Error opening socket" );
    return 1;
  }
  d( "Socket fd: %d\n", *sock );
  /* bind */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;                  //IPv4
  addr.sin_port = htons( port );
  addr.sin_addr.s_addr = htonl( INADDR_ANY );  //any local IP is good
  if ( bind( *sock, ( struct sockaddr* ) &addr,
             sizeof( struct sockaddr_in ) ) == -1 ) {
    perror( "Error while binding socket" );
    return 1;
  }
  return 0;
}

/**
 * @brief A structure for handling IP addresses
 */
struct EndPoint {
  union {
    struct sockaddr *base;
    struct sockaddr_in *in;
  } addr;
  socklen_t len;
};

/**
 * @brief sends a packet over a UDP socket
 *
 * @param sock the UDP socket to send the packet on
 * @param target the target \em EndPoint i.e. IP address
 * @param packet the packet to be sent
 * @param local the local address
 *
 * @return the number of bytes successfully sent
 */
int send_packet( int sock, struct EndPoint *target, char *packet,
                 struct sockaddr *local )
{
  print_packet( ( struct sockaddr_in * ) local,
                ( struct sockaddr_in * ) target->addr.in, packet, SEND );

  size_t packetSize = HEADER_SIZE + GETLEN( packet );
#ifdef PL_DENOMINATOR
  FILE *urand = fopen( "/dev/urandom", "r" );
  if ( fgetc( urand ) % PL_DENOMINATOR ) {
    fclose( urand );
    d( "losing this packet!\n" );
    return 0;
  }
  fclose( urand );
#endif
  return sendto( sock, packet, packetSize, 0, target->addr.base,
                 target->len ) != packetSize;
}

/**
 * @brief send all packets in a linked list of packets
 *
 * @param sock the UDP socket to send the packet on
 * @param target the target IP address
 * @param packets the list of packets
 * @param local the local address
 *
 * @return 0 on success, non zero otherwise
 */
int send_all( int sock, struct EndPoint *target, struct List *packets,
              struct sockaddr *local )
{
  if ( packets->head == NULL ) {
    return 0;
  }
  for ( struct ListNode *n = packets->head; n != NULL; n = n->next ) {
    if ( send_packet( sock, target, ( char * ) n->data, local ) ) {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief create an SRTP packet structure
 *
 * @param command the packet command
 * @param sequence the sequence of the packet
 * @param payloadSize the payload size
 * @param payload the payload data
 *
 * @return a pointer to a buffer containing the newly formed packet
 */
char *create_packet( uint8_t command, uint16_t sequence, uint16_t payloadSize,
                     char *payload )
{
  char *buffer = malloc( sizeof( char ) * ( payloadSize + HEADER_SIZE ) );
  buffer[0] = command;
  set_seq_num( buffer, sequence );
  SETLEN( buffer, payloadSize );
  if ( payload != NULL ) {
    memcpy( buffer + HEADER_SIZE, payload, payloadSize );
  }
  return buffer;
}

/**
 * @brief compare two address
 *
 * @param a address a
 * @param b address b
 *
 * @return non-zero if they are equal, 0 otherwise
 */
int compare_addr( struct sockaddr_in *a, struct sockaddr_in *b )
{
  fprintf( stderr, "a: %s\n", inet_ntoa( a->sin_addr ) );
  fprintf( stderr, "b: %s\n", inet_ntoa( b->sin_addr ) );
  return a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr;
}

/**
 * @brief read a packet from only the address specified
 *
 * @param sock the UDP socket to recv on
 * @param buffer the buffer to place the packet
 * @param packetSize the maximum packet size
 * @param flags UDP socket flags
 * @param wanted the target address
 * @param wantedSize the target address structure size
 *
 * @return the number of bytes successfully read, less than zero otherwise
 */
ssize_t read_only_from( int sock, char *buffer, size_t packetSize, int flags,
                        struct sockaddr *wanted, socklen_t *wantedSize )
{
  ssize_t read = 0;
  char *temp = malloc( sizeof( char ) * packetSize );
  if ( temp == NULL ) {
    return -1;
  }
  struct sockaddr from;
  socklen_t fromSize;
  read = recvfrom( sock, temp, packetSize, flags, &from, &fromSize );
  // is this from the endpoint we care about?
  if ( read > 0 && ( 1 || compare_addr( ( struct sockaddr_in* ) &from, ( struct sockaddr_in* ) wanted ) ) ) {
    memcpy( buffer, temp, packetSize );
    free( temp );
    return read;
  } else if ( read > 0 ) {
    d( "Discarding foreign packet\n" );
  }

  //no? well, ignore it
  free( temp );
  errno = EAGAIN;
  return -1;
}

/**
 * @brief wait for a packet until a timeout occurs
 *
 * @param socket the UDP socket to wait on
 * @param buffer the buffer to place the packet
 * @param packetSize the maximum packet size
 * @param flags UDP socket flags
 * @param end the target address to recv from
 *
 * @return
 */
ssize_t read_until_timeout( int socket, char *buffer, size_t packetSize, int flags,
                            struct EndPoint *end )
{
  time_t startTime = time( NULL );
  ssize_t read;
  while ( time( NULL ) - startTime < 6 ) {
    read = read_only_from( socket, buffer, PACKET_SIZE, MSG_DONTWAIT,
                           end->addr.base, &end->len );
    if ( read > 0 ) {
      return read;
    }
    usleep( 10000 );
  }
  d( "Connection timed out\n" );
  return -1;
}

#endif
