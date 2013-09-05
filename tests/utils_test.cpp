#include "gtest/gtest.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/socket.h>

#include "../srtp/srtp_impl.h"
#include "../srtp/srtp_debug.h"

#define TEST_MESSAGE "Hello World!\r\n"
#define TEST_MESG_LEN (14)

TEST(PacketTest, PacketMacros) {
  
  int n;
  char buffer[ 2048 ];
  char s[128];
  
  EXPECT_EQ(PKT_HEADERSIZE, (size_t)8);
  debug("[PacketMacros] PKT_HEADERSIZE = %d\n", PKT_HEADERSIZE);
  
  EXPECT_EQ(pkt_set_payload(buffer, (char*)TEST_MESSAGE, (size_t)TEST_MESG_LEN), TEST_MESG_LEN);
  EXPECT_EQ(pkt_set_payload(buffer, (char*)TEST_MESSAGE, (size_t)0), 0);
  EXPECT_LT(pkt_set_payload(buffer, (char*)TEST_MESSAGE, (size_t)1025), 0);
  
  srtp_cmdstr(FIN|ACK,s);
  EXPECT_STREQ(s,"FIN+ACK");
  srtp_cmdstr(SYN|RST,s);
  EXPECT_STREQ(s,"RST+SYN");
  
  //EXPECT_[GT|LT|EQ|STREQ](,)
}
