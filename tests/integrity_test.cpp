#include "gtest/gtest.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/socket.h>

#include "../srtp/srtp.h"

#define TEST_MESSAGE "Hello World!\r\n"
#define TEST_MESG_LEN (14)
#define TEST_PORT ( 8080 )


TEST(IntegrityTest, ExistingFifo) {

  int fd = open( "tmpfifo00000", O_CREAT | O_WRONLY );
  close( fd );

  EXPECT_GT( srtp_socket( AF_INET, SOCK_STREAM, 0 ), 0 );

  unlink( "tmpfifo00000" );

}

TEST(IntegrityTest, InvalidSocket) {

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( 3000 );
  addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

  EXPECT_LT( srtp_listen( -1, 0 ) , 0 );

  EXPECT_LT( srtp_bind(-1, ( struct sockaddr* ) &addr , ( socklen_t ) sizeof(  struct sockaddr_in ) ) , 0 );

  EXPECT_LT( srtp_connect( -1, ( struct sockaddr* ) &addr, ( socklen_t ) sizeof( struct sockaddr_in ) ), 0 );

  EXPECT_LT( srtp_accept( -1, NULL, NULL) , 0 );

  EXPECT_LT( srtp_shutdown( -1, NULL) , 0 );

  int sock = srtp_socket( AF_INET, SOCK_STREAM, 0 );

  EXPECT_EQ( srtp_bind(sock, ( struct sockaddr* ) &addr , ( socklen_t ) sizeof(  struct sockaddr_in ) ) , 0 );

  close( sock );

  EXPECT_LT( srtp_bind(sock, ( struct sockaddr* ) &addr , ( socklen_t ) sizeof(  struct sockaddr_in ) ), 0 );

  EXPECT_LT( srtp_connect(sock, ( struct sockaddr* ) &addr , ( socklen_t ) sizeof(  struct sockaddr_in ) ) , 0 );

  EXPECT_LT( srtp_shutdown(sock, NULL) , 0 );

  EXPECT_LT( srtp_accept(sock, NULL, NULL) , 0 );

  EXPECT_LT( srtp_listen(sock , 0 ) , 0 );

}

TEST(IntegrityTest, InvalidAddress) {

  int sock = srtp_socket( AF_INET, SOCK_STREAM, 0 );

  EXPECT_LT( srtp_connect( sock, NULL, NULL ) , 0 );

  EXPECT_LT( srtp_bind( sock, NULL, NULL ) , 0 );

}

TEST(IntegrityTest, UnboundListen) {

  int sock = srtp_socket( AF_INET, SOCK_STREAM, 0 );

  EXPECT_GT( sock, 0 );

  EXPECT_LT( srtp_listen(sock , 0 ) , 0 );

}

TEST(IntegrityTest, BoundListen) {

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( 3000 );
  addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

  int sock = srtp_socket( AF_INET, SOCK_STREAM, 0 );

  EXPECT_GT( sock, 0 );

  EXPECT_EQ( srtp_bind(sock, ( struct sockaddr* )&addr , ( socklen_t ) sizeof(  struct sockaddr_in ) ), 0 );

  EXPECT_EQ( srtp_listen(sock, 0 ) , 0 );

}

