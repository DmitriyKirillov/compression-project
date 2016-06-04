#include <library/Bor/Bor.h>
#include <experimental/string_view>
#include <iostream>
#include <string>
#include <vector>
// #include <library/tests_common/tests_common.h>

/*TEST(HuffmanCodecTest, Works) {
    Codecs::HuffmanCodec codec;
    Codecs::TestSimple(codec);
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/

int main() {
    Codecs::BOR codec;

    codec.learn(
            {"Lorem ipsum dolor sit amet, consectetur ababa adipisicing elit, "
                     "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
                     "Ut ababa enim ad minim veniam, quis ababac nostrud exercitation ullamco ba laboris"
                     "nisi ut aliquip ex ea commodo ababa consequat. Duis aute ababa irure dolor in reprehenderit in"
                     "voluptate velit esse ea cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat"
                     "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."});

    return 0;
}
