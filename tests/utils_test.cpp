#include "gtest/gtest.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/socket.h>

#include "../srtp/srtp_util.h"
#include "../srtp/srtp_debug.h"

#define TEST_MESSAGE "Hello World!\r\n"

TEST(PacketTest, PacketMacros) {
  
  int n;
  char buffer[ 2048 ];
  
  EXPECT_EQ(PKT_HEADERSIZE, (size_t)8);
  debug("[PacketMacros] PKT_HEADERSIZE = %d\n", PKT_HEADERSIZE);
  
  //EXPECT_[GT|LT|EQ|STREQ](,)
}