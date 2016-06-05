#include <library/DictHuffman/DictHuffman.h>
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
    Codecs::DictHuffmanCodec codec;

    std::string raw = "Lorem ipsum dolor sit amet, consectetur ababa adipisicing elit, "
            "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
            "Ut ababa enim ad minim veniam, quis ababac nostrud exercitation ullamco ba laboris"
            "nisi ut aliquip ex ea commodo ababa consequat. Duis aute ababa irure dolor in reprehenderit in"
            "voluptate velit esse ea cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat"
            "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    codec.learn({raw});
    std::string enc;
    std::string dec;
    codec.encode(enc, raw);
    codec.decode(dec, enc);
    std::cout << raw << '\n' << dec << '\n';

    return 0;
}
