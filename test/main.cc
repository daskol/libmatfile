//  main.cc

#include <gtest/gtest.h>

TEST(SanityCheck, EmptyTest) {
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
