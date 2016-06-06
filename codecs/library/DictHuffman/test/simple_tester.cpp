#include <library/DictHuffman/DictHuffman.h>
#include <library/Huffman/Huffman.h>
#include <experimental/string_view>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>


int main() {
    Codecs::DictHuffmanCodec codec;
    size_t sample_size = codec.sample_size();

    std::ifstream in;
    in.open("/home/dmitry/compression-project/test-data/data1");

    std::vector<std::experimental::string_view> sample;
    sample.reserve(sample_size);
    std::string buffer;
    for (size_t i = 0;i < sample_size && getline(in, buffer); ++i) {
        sample.push_back(buffer);
        buffer.clear();
    }

    std::ofstream deb("/home/dmitry/compression-project/test-data/simple.txt", std::ios::trunc);
    for (size_t j = 0; j < sample.size(); ++j) {
        deb << sample[j];
    }
    deb.close();

    std::cout << "Start Learning" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    codec.learn(sample);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    std::cout << "Learning Finished for " << duration << " nanoseconds.\n Start encoding" << std::endl;
    std::istreambuf_iterator<char> eos;
    std::string raw(std::istreambuf_iterator<char>(in), eos);
    raw.resize(300000);
    std::string enc;
    codec.encode(enc, raw);
    std::cout << "Compression Finised. Compression Ratio is " <<
            static_cast<long double>(enc.size()) / static_cast<long double>(raw.size()) << std::endl;

    std::string decoded;
    start = std::chrono::high_resolution_clock::now();
    codec.decode(decoded, enc);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    std::cout << "Decoding finished in " << static_cast<long double>(duration) / 100000000 << " seconds.\n";
    if (decoded == raw) {
        std::cout << "Data decoded correctly\n";
    } else {
        std::cout << "Data decoded incorrectly\n";
        std::cout << raw.size() << "  " << decoded.size() << "\n\n";
        size_t bad = 0;
        for (size_t i = 0; i < std::min(raw.size(), decoded.size()); ++i) {
            if (raw[i] != decoded[i]) {
                bad = i;
                break;
            }
        }
        std::cout << std::string(raw.begin() + bad, raw.end()) << "\n\n\n" << std::string(decoded.begin() + bad, decoded.end());
        std::cout << "\n\n" << *raw.rbegin() << ' ' << *decoded.rbegin() << '\n';
    }

    in.close();
    return 0;
}