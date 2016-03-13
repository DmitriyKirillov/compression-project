#include <library/Huffman/Huffman.h>
#include <bitset>
#include <string>
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

    codec.learn({"Hello, World. We can construct a model now"});

    std::cout << codec.save() << '\n';
    return 0;
}
