#include <external/optionparser/optionparser.h>
#include <library/DictHuffman/DictHuffman.h>
#include <library/common/sample.h>

#include <experimental/string_view>
#include <algorithm>
#include <cstring>
#include <thread>
#include <fstream>
#include <map>
#include <chrono>
#include <random>
#include <set>
#include <vector>
#include <stdlib.h>
#include <string>

enum optionIndex {
    UNKNOWN, HELP, INPUT_FILE, INPUT_TYPE, TINY, S_SIZE, NO_SHUFFLE
};
const option::Descriptor usage[] =
        {
                {UNKNOWN,     0, "",  "",            option::Arg::None,     ""},
                {HELP,        0, "h", "help",        option::Arg::None,     ""},
                {INPUT_FILE,  0, "",  "test-file",   option::Arg::Optional, ""},
                {INPUT_TYPE,  0, "t", "",            option::Arg::Optional, ""},
                {TINY,        0, "",  "sm",          option::Arg::Optional, ""},
                {S_SIZE,      0, "",  "sample-size", option::Arg::Optional, ""},
                {NO_SHUFFLE,  0, "",  "no-shuffle",  option::Arg::None,     ""},
                {0,           0, 0,   0,             0,                     0}
        };

size_t read_entries(std::istream &in, std::vector<std::string> &data, int_fast64_t n, bool LE_type = false) {
    size_t readed = 0;
    if (LE_type) {
        in.seekg(0, in.end);
        long length = in.tellg();
        in.seekg(0, in.beg);

        char *char_buff = new char[length];
        in.read(char_buff, length);

        int_fast64_t i = 0;
        uint_fast32_t entry_size;
        while (i != length && i != n) {
            entry_size = 0;
            for (int j = 0; j < 4; ++j) {
                entry_size += (static_cast<unsigned char>(char_buff[i + j]) << (j * 8));
            }
            i += 4;
            data.push_back(std::string(char_buff + i, entry_size));
            readed += entry_size;
            i += entry_size;
        }
        delete[] char_buff;
    } else {
        std::string buff;
        for (int_fast64_t i = 0; i != n && getline(in, buff); ++i) {
            data.push_back(buff);
            readed += buff.size();
            buff.clear();
        }
    }
    return readed;
}

size_t SMALL_ENTRY_NUM = 1000;

void correct_formating(const char *src_filename, const char *dst_filename, bool enc_correct) {
    std::ifstream src;
    src.open(src_filename);
    std::vector<std::string> data;
    read_entries(src, data, 0, enc_correct);
    src.close();
    uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(data.begin(), data.end(), std::default_random_engine(seed));
    std::ofstream norm_data;
    norm_data.open(dst_filename);
    for (size_t j = 0; j < data.size(); ++j) {
        norm_data << data[j] << (enc_correct ? "\n" : "");
    }
    norm_data.close();
    norm_data.clear();
}

int main(int argc, char *argv[]) {
    argc -= (argc > 0);
    argv += (argc > 0); // skip program name argv[0] if present
    option::Stats stats(usage, argc, argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    if (parse.error())
        return 1;

    if (options[UNKNOWN]) {
        std::cout << "Unknown option '" << options[UNKNOWN].name << "' use --help for help.\n";
        return 0;
    }
    if (options[HELP]) {
        std::cout << "USAGE: ./tester --test-file=test.txt [options]\n"
                "Options:\n-h, --help\n\t\tThis help page\n\n"
                "--test-file=<path>\n\t\tfull path to the file with test data\n\n"
                "-t\n\t\tType of file encoding. use -tLE if data file's entry looks like 'LE uint32 size + entry'\n\n"
                "--sm=<number>\n\t\tCodec will encode only first <number> entries from test file."
                " Tester will show compresion ration, but not compression time\n\n"
                "--sample-size=<new size>\n\t\tTester will use <new size> entries to train codec\n\n"
                "--no-shuffle\n\t\tTester won't shuffle test data before learning. !!! It can reduce compression ratio\n";
        return 0;
    }

    if (!options[INPUT_FILE] || options[INPUT_FILE].arg == nullptr) {
        std::cout << "You have to enter test file path. Usage:\n ./tester --test-file=/tmp/test.txt\n";
        return 0;
    }

    bool LE_encoding = false;
    if (options[INPUT_TYPE].arg != nullptr) {
        std::string tmp(options[INPUT_TYPE].arg);
        LE_encoding = (tmp == "LE");
    }

    if (options[TINY]) {
        if (options[TINY].arg != nullptr) {
            SMALL_ENTRY_NUM = std::stoi(options[TINY].arg);
        }
        std::cout << "In this mode codec will encode only " << SMALL_ENTRY_NUM << " entries from file. Tester won't"
                " show any encoding speed information (because it will bi incorrect), however tester will show correct compression ratio.\n";
    }
    size_t sample_size = 0;
    if (options[S_SIZE]) {
        if (options[S_SIZE].arg != nullptr) {
            sample_size = std::stoi(options[S_SIZE].arg);
        } else {
            std::cout << "After --sample-size you should enter new size of sample. Ex.: --sample-size=100\n";
        }
    }

    std::cout << "Preparing test file. It can take some time\n";

    std::fstream norm_file(".test.data", std::fstream::in | std::fstream::out | std::ios::trunc);
    {
        std::ifstream src;
        src.open(options[INPUT_FILE].arg);
        std::vector<std::string> data;
        read_entries(src, data, -1, LE_encoding);
        src.close();
        if (!options[NO_SHUFFLE]) {
            uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(data.begin(), data.end(), std::default_random_engine(seed));
        }
        for (size_t j = 0; j < data.size(); ++j) {
            norm_file << data[j] << '\n';
        }
    }
    norm_file.close();
    norm_file.clear();

    Codecs::DictHuffmanCodec codec;
    norm_file.open(".test.data");

    sample_size = (sample_size ? sample_size : codec.sample_size());
    std::vector<std::experimental::string_view> sample;
    std::string buff;
    for (size_t i = 0; i < sample_size && getline(norm_file, buff); ++i) {
        sample.push_back(buff);
        buff.resize(0);
    }

    std::cout << "Start Learning speed test\n";

    auto start = std::chrono::high_resolution_clock::now();
    codec.learn(sample);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    std::cout << "Learning finished in " << static_cast<long double>(duration) / 1000000000 << " seconds.\n";

    norm_file.clear();
    norm_file.seekg(0, norm_file.beg);
    std::string raw;
    if (options[TINY]) {
        for (size_t i = 0; i < SMALL_ENTRY_NUM && getline(norm_file, buff); ++i) {
            raw.append(buff);
            buff.clear();
        }
    } else {
        std::istreambuf_iterator<char> eos;
        raw = std::string(std::istreambuf_iterator<char>(norm_file), eos);
    }
    norm_file.close();

    std::string enc;
    std::cout << "Start encoding test.";
    if (!options[TINY]) {
        std::cout << " I'm sorry, but you have to wait for a long time :(\t\tUse --sm flag to wait less.";
    }
    std::cout << '\n';
    start = std::chrono::high_resolution_clock::now();
    codec.encode(enc, raw);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    if (!options[TINY]) {
        std::cout << "Encoding finished in " << static_cast<long double>(duration) / 100000000 << " seconds.\n";
    }
    std::cout << "Compression ratio is: " << (long double) 1 - (long double) (enc.size()) / (long double) raw.size()
    << "\n Start decoding test\n";

    std::string decoded;
    start = std::chrono::high_resolution_clock::now();
    codec.decode(decoded, enc);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    if (!options[TINY]) {
        std::cout << "Decoding finished in " << static_cast<long double>(duration) / 100000000 << " seconds.\n";
    }
    if (decoded == raw) {
        std::cout << "Data decoded correctly\n";
    } else {
        std::cout << "Data decoded incorrectly\n";
    }
    std::remove(".test.data");

    return 0;
}