#include "baudvine/mytex.h"

#include <gtest/gtest.h>

TEST(Mytex, BasicCreateLockDestroy) {
  baudvine::Mytex<int> underTest(5); 
  EXPECT_EQ(*underTest.Lock(), 5);
  *underTest.Lock() = 6;
  EXPECT_EQ(*underTest.Lock(), 6);
}
