#include <unity.h>

void test_sanity() {
  TEST_ASSERT_TRUE(true);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sanity);
  UNITY_END();
}
