#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void example_test()
{
    TEST_ASSERT(1+1==2);
    TEST_ASSERT_EQUAL(2,1+1);
    TEST_ASSERT_NOT_EQUAL(3,1+1);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(example_test);
    UNITY_END();
}