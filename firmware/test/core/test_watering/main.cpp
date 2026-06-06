#include <unity.h>
#include <iostream>

void setUp() {}
void tearDown() {}

void test_compute() {
  std::cout << "ff" << std::endl;
  TEST_ASSERT_EQUAL(10, 10);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_compute);
  return UNITY_END();
}
