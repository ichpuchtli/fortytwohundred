#include "gtest/gtest.h"

TEST( TypeIntegrityTest, 1eq1 ) {

  EXPECT_EQ( 1, 1 );

}

TEST( TypeIntegrityTest, Null) {

  EXPECT_FALSE( NULL );

  EXPECT_TRUE( !NULL );

}

TEST( TypeIntegrityTest, String ) {

  char* str = NULL;

  EXPECT_STREQ( NULL, str );

  str = "Hello World!";

  EXPECT_STREQ( "Hello World!" , str );
}
