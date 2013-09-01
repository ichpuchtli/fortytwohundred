#include "gtest/gtest.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/socket.h>

#include "../srtp/srtp.h"
#include "../srtp/srtp_util.h"

#define TEST_MESSAGE "Hello World!\r\n"

TEST(PacketTest, PacketPointers) {
  
  int n;
  char buffer[ 2048 ];
  
  debug("[Packet-Tests] headerSize=%d\n", PKT_HEADERSIZE);
  
  
  //EXPECT_[GT|LT|EQ|STREQ](,)
}