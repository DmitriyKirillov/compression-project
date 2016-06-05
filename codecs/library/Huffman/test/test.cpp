#include <library/Huffman/Huffman.h>
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
    Codecs::HuffmanCodec codec;

    codec.learn(
            {"Lorem ipsum dolor sit amet, consectetur adipisicing elit, "
                     "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
                     "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris"
                     "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in"
                     "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat"
                     "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."});

    std::string simple_raw = "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris";

    std::string code;
    codec.encode(code, simple_raw);
    std::string decoded;
    codec.decode(decoded, code);
    std::cout << ((decoded == simple_raw) ? ("Matched") : ("Didn't matched")) << '\n' << simple_raw
    << "\nCompression ratio: " << static_cast<double>(simple_raw.size()) / static_cast<double>(code.size())
    << '\n' << decoded;

    Codecs::HuffmanCodec other;
    other.load(codec.save());

    std::string code_new;
    other.encode(code_new, simple_raw);
    std::cout << "\n\nDictionary copied ";
    if (code_new == code)
        std::cout << "successfully";
    else
        std::cout << "with erorrs";
    std::cout << std::endl;

    return 0;
}
