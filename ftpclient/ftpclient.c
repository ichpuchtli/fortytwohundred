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

void signal_handler( int signal )
{
  fprintf( stderr, "SIGPIPE received, exiting\n" );
  exit( RUNTIME_ERROR );
}

/**
 * @brief get the file size of a file
 *
 * @param filename the file path of the file
 *
 * @return the size in bytes of the file
 */
long get_file_size( char *filename )
{
  struct stat s;
  stat( filename, &s );
  return s.st_size;
}

/**
 * @brief establish a SRTP connection with a host
 *
 * @param sock the UDP socket to communicate on
 * @param buffer the buffer to store send/recv data
 * @param size the maximum size of the buffer
 * @param filename the filename to store on the server
 * @param target the target address of the host
 * @param mine the local address
 */
void complete_request( int sock, char *buffer, long size, char *filename,
                       struct EndPoint *target, struct sockaddr *mine )
{
  char *payload = buffer + HEADER_SIZE;
  int size_read = 0;
  time_t startTime = time( NULL );

  ////////////////////////////////////////////////////////////////////////////////
  /// Connection Establishment
  sprintf( payload, "%s|%ld", filename, size );
  char *request = create_packet( CMD_WRQ, 0, strlen( payload ), payload );
  do {
    d( "Sending request\n" );
    if ( send_packet( sock, target, request, mine ) ) {
      perror( "Error sending request" );
      exit( RUNTIME_ERROR );
    }
    usleep( 100000 );
    size_read = recvfrom( sock, buffer, PACKET_SIZE, MSG_DONTWAIT,
                          target->addr.base, &target->len );
  } while ( size_read < 0 && time( NULL ) - startTime < 6 );
  free( request );

  if ( size_read < 1 ) {
    fprintf( stderr, "Connection to server timed out.\n" );
    exit( TIMEOUT );
  }
  print_packet( ( struct sockaddr_in * ) mine, target->addr.in, buffer, RECV );

  if ( buffer[0] == CMD_ACK ) {
    d( "Request accepted\n" );
  } else if ( buffer[0] == CMD_EXISTS ) {
    printf( "%s already exists on the server, "
            "the write request was rejected\n", filename );
    exit( FILE_EXISTS );
  } else if ( buffer[0] == CMD_BADREQ ) {
    fprintf( stderr, "Server rejected request as badly formed\n" );
    exit( BAD_REQUEST );
  }

}


/**
 * @brief transfer data to the ftpserver until all data is acknowledged
 *
 * @param file the file to copy
 * @param sock the UDP socket to transfer the data on
 * @param filename the filename of the file
 * @param target the target address of the ftpserver
 *
 * @return 0 on success, non zero otherwise
 */
int copy_file( FILE *file, int sock, char *filename, struct EndPoint *target )
{
  char buffer[PACKET_SIZE] = {0};
  char *payload = buffer + HEADER_SIZE;

  long size = get_file_size( filename );
  struct sockaddr mine;

  socklen_t mySize;
  if ( getsockname( sock, &mine, &mySize ) == -1 ) {
    perror( "getting sock name:" );
  }
  // make our request to the server, exits if timeout
  complete_request( sock, buffer, size, filename, target, &mine );

  // set up for data transfer
  long written = 0, read = 0;
  uint16_t sequence = 1;
  struct List *packets = malloc( sizeof( struct List ) );
  init_list( packets );

  time_t lastACK = time( NULL );

  ////////////////////////////////////////////////////////////////////////////////
  /// Data Transfer
  while ( written != size ) {
    /* read enough for our window */
    while ( packets->size < WINDOW_SIZE && written < size ) {

      size_t remaining = size - read < PAYLOAD_SIZE ?
                         size - read : PAYLOAD_SIZE;

      size_t chunksize = fread( payload, sizeof( char ), remaining, file );

      if ( chunksize == 0 ) {
        break;
      }

      add_to_list( packets,
                   create_packet( CMD_DATA, sequence++, chunksize, payload ) );
      read += chunksize;
    }
    /* send the whole window's worth */
    if ( send_all( sock, target, packets, &mine ) ) {
      perror( "Error sending data packet" );
      exit( RUNTIME_ERROR );
    }

    if ( ( time( NULL ) - lastACK ) > ACK_TIMEOUT ) {
      fprintf( stderr, "Connection timed out\n" );
      exit( TIMEOUT );
    }

    usleep( 100000 );

    while ( 1 ) {

      read = read_only_from( sock, buffer, PACKET_SIZE, MSG_DONTWAIT, target->addr.base, &target->len );

      if ( read < 0 && ( errno == EWOULDBLOCK || errno == EAGAIN ) ) {
        break;
      }

      print_packet( ( struct sockaddr_in * )&mine, target->addr.in, buffer, RECV );

      uint16_t acked_seq = get_seq_num( buffer );

      if ( buffer[0] == CMD_ACK ) {


        while ( packets->head != NULL ) {

          uint16_t f = get_seq_num( ( char* ) packets->head->data );
          uint16_t l = get_seq_num( ( char* ) packets->tail->data );

          if ( acked_seq == f ) {
            break;
          }

          int inside = f <= l;
          if ( ( inside && acked_seq < f && acked_seq > l )
               || ( !inside && acked_seq > f && acked_seq < l ) ) {
            break;
          }
          lastACK = time( NULL );
          written += GETLEN( ( char * )packets->head->data );
          free( packets->head->data );
          remove_front( packets );
        }
      } else if ( buffer[ 0 ] == CMD_FIN ) {
        if ( written < size ) {
          fprintf( stderr, "Server closed connection unexpectedly\n" );
          exit( BAD_REQUEST );
        } else {
          break;
        }
      } else {
        d( "Received unknown packet command\n" );
      }
    }
  }
  d( "closing connection\n" );
  ////////////////////////////////////////////////////////////////////////////////
  char *packet = create_packet( CMD_FIN, 0, 0, NULL );
  ssize_t r;
  usleep( 10000 );
  lastACK = time( NULL );
  while ( time( NULL ) - lastACK < ACK_TIMEOUT ) {
    send_packet( sock, target, packet, &mine );
    usleep( 100000 );
    r = read_only_from( sock, buffer, PACKET_SIZE, MSG_DONTWAIT,
                        target->addr.base, &target->len );
    // ignore leftover ACKs from the data packets
    while ( r > 0 && buffer[0] == CMD_ACK ) {
      print_packet( ( struct sockaddr_in * )&mine, target->addr.in, buffer, RECV );
      r = read_only_from( sock, buffer, PACKET_SIZE, MSG_DONTWAIT,
                          target->addr.base, &target->len );
    }
    print_packet( ( struct sockaddr_in * )&mine, target->addr.in, buffer, RECV );
    if ( buffer[0] == CMD_FINACK ) {
      // FIN has been ACKed
      break;
    }
  }

  free( packet );
  d( "File transfer complete\n" );
  return 0;
}

/**
 * @brief resolve a host address
 *
 * @param addr the address structure to store the address in
 * @param len the length of the address structure
 * @param hostname th host name of the host
 * @param port the port to connect it to
 *
 * @return 0 on success, non zero otherwise
 */
int process_address( struct sockaddr **addr, socklen_t *len,
                     char *hostname, char *port )
{
  struct addrinfo hints;
  struct addrinfo *result = NULL;
  memset( &hints, 0, sizeof( struct addrinfo ) );
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  int ai = -1;
  if ( ( ai = getaddrinfo( hostname, port, &hints, &result ) ) != 0 ) {
    fprintf( stderr, "Error getting address info: %s\n", gai_strerror( ai ) );
    exit( BAD_PORT );
  }
  *addr = result->ai_addr;
  *len = result->ai_addrlen;
  return 0;
}

/**
 * @brief check the arguments reseted to the application
 *
 * @param argc argument count
 * @param argv argument values
 *
 * @return 0 on success, non zero otherwise
 */
int arg_check( int argc, char **argv )
{
  if ( argc == 5 ) {
    /* -d not supplied */
    if ( strncmp( "-d", argv[1], 2 ) != 0 ) {
      fprintf( stderr, USAGE, argv[0] );
      return BAD_ARGS;
    }
    debug = 1;
  } else if ( argc == 4 ) {
    /* -d supplied */
    if ( strncmp( "-d", argv[1], 2 ) == 0 ) {
      fprintf( stderr, USAGE, argv[0] );
      return BAD_ARGS;
    }
  } else {
    fprintf( stderr, USAGE, argv[0] );
    return BAD_ARGS;
  }
  return 0;
}

int main( int argc, char **argv )
{
  unsigned int port = 0;
  int args = arg_check( argc, argv );
  if ( args ) {
    return BAD_ARGS;
  }
  FILE *file = NULL;
  /* check the port */
  char crap;
  if ( ( sscanf( argv[2 + debug], "%u%c", &port, &crap ) ) != 1 || port < 1
       || port > 65535 ) {
    fprintf( stderr, "The port '%s' is invalid, please provide a "
             "port in the range 1 to 65535\n", argv[2 + debug] );
    return BAD_PORT;
  }
  d( "Port OK: %d\n", port );
  /* check the file */
  if ( ( file = fopen( argv[3 + debug], "rb" ) ) == NULL ) {
    perror( "Error opening given file" );
    return BAD_FILE;
  }
  d( "File opened successfully\n" );
  /* set up the signal handler */
  struct sigaction signals;
  signals.sa_handler = signal_handler;
  signals.sa_flags = SA_RESTART;
  sigaction( SIGPIPE, &signals, NULL );

  /* check address, set up socket, connect */
  struct EndPoint target;
  process_address( &target.addr.base, &target.len, argv[1 + debug],
                   argv[2 + debug] );
  d( "getaddrinfo() successful\n" );

  int sock;
  if ( setup_socket( &sock, 0 ) != 0 ) {
    return RUNTIME_ERROR;
  }
  d( "Bound successfully\n" );
  /* request to write (reads not required in the assignment) */
  copy_file( file, sock, argv[3 + debug], &target );
  d( "Exiting without errors\n" );
  fclose( file );
  close( sock );
  return 0;
}
