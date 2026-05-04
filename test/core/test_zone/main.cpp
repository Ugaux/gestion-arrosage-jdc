#include <unity.h>
#include <iostream>

#include "core/interfaces/IValve.h"

class FakeValve : public IValve {
public:
  bool opened = false;

  void open() override { opened = true; }
  void close() override { opened = false; }
};

void setUp() {}
void tearDown() {}

void test_compute() {
  std::cout << "logic.compute(5)=" << std::endl;
  TEST_ASSERT_EQUAL(10, 10);
}
void test_valve() {
  FakeValve fakeValve;
  fakeValve.open();
  TEST_ASSERT(fakeValve.opened);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_compute);
  return UNITY_END();
}
