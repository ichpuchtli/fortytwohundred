#include "gtest/gtest.h"

#include "../srtp/srtp.h"

TEST( SocketTest, IPv6_Test ) {

  int sock_fd;

  sock_fd = srtp_socket(AF_INET6, SOCK_STREAM, 0);

  EXPECT_TRUE( sock_fd > 0 );

  sock_fd = srtp_socket(AF_INET6, SOCK_DGRAM, 0);

  EXPECT_TRUE( sock_fd > 0 );

}

TEST( SocketTest, IPv4_Test ) {

  int sock_fd;

  sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  EXPECT_TRUE( sock_fd > 0 );

  sock_fd = srtp_socket(AF_INET, SOCK_DGRAM, 0);

  EXPECT_TRUE( sock_fd > 0 );

}

TEST( SocketTest, Argument_Test) {

  int sock_fd = srtp_socket(-1, -1, -1);

  EXPECT_TRUE( sock_fd < 0 );

}
