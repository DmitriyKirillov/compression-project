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
#include <stdlib.h>
#include <string>

enum optionIndex {
    UNKNOWN, HELP, INPUT_FILE, OUTPUT_FILE, INPUT_TYPE, TINY
};
const option::Descriptor usage[] =
        {
                {UNKNOWN,     0, "",  "",          option::Arg::None,     "USAGE: example [options]\n\n"
                                                                                  "Options:"},
                {HELP,        0, "h", "help",      option::Arg::None,     "  --help  \tPrint usage and exit."},
                {INPUT_FILE,  0, "",  "test-file", option::Arg::Optional, ""},
                {OUTPUT_FILE, 0, "",  "output",    option::Arg::Optional, ""},
                {INPUT_TYPE,  0, "t", "",          option::Arg::Optional, ""},
                {TINY,        0, "",  "sm",        option::Arg::Optional, ""},
                {0,           0, 0,   0,           0,                     0}
        };

size_t count_entries(std::istream &in, bool LE_type = false) {
    size_t counter = 0;
    if (LE_type) {
        uint32_t entry_size;
        char int_buff[5];
        while (in.good()) {
            in.read(int_buff, 4);
            int_buff[4] = int_buff[0];
            int_buff[0] = int_buff[3];
            int_buff[3] = int_buff[4];

            int_buff[4] = int_buff[1];
            int_buff[1] = int_buff[2];
            int_buff[2] = int_buff[4];

            std::memcpy(&entry_size, int_buff, 4);
            std::cout << int_buff << '\n';
            in.seekg(entry_size, std::ios::cur);
        }
    } else {
        std::string buff;
        while (getline(in, buff)) {
            buff.resize(0);
            ++counter;
        }
    }
    return counter;
}

size_t read_entries(std::istream &in, std::vector<std::string> &data, size_t n, bool LE_type = false) {
    data.reserve(data.size() + n);
    size_t readed = 0;
    if (LE_type) {
        uint32_t entry_size;
        char int_buff[5];
        char *char_buff;
        for (size_t i = (n == 0); i != n && in; ++i) {
            in.read(int_buff, 4);
            int_buff[4] = int_buff[0];
            int_buff[0] = int_buff[3];
            int_buff[3] = int_buff[4];

            int_buff[4] = int_buff[1];
            int_buff[1] = int_buff[2];
            int_buff[2] = int_buff[4];

            std::memcpy(&entry_size, int_buff, 4);
            char_buff = new char[entry_size];
            in.read(char_buff, entry_size);
            data.push_back(std::string(char_buff, entry_size));
            readed += entry_size;
            delete[] char_buff;
        }
    } else {
        std::string buff;
        for (size_t i = (n == 0); i != n && getline(in, buff); ++i) {
            data.push_back(buff);
            readed += buff.size();
            buff.resize(0);
        }
    }
    return readed;
}

void pickup_random_entries(std::istream &in, std::vector<std::string> &out, size_t n, size_t max_entries, bool LE_type=false) {
    std::set<uint_fast32_t> numbers;
    std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
    while (numbers.size() < n) {
        numbers.insert(generator() % max_entries);
    }

    size_t counter = 0;
    if (LE_type) {
        uint32_t entry_size;
        char int_buff[5];
        char *char_buff;
        while (in.good()) {
            in.read(int_buff, 4);
            int_buff[4] = int_buff[0];
            int_buff[0] = int_buff[3];
            int_buff[3] = int_buff[4];

            int_buff[4] = int_buff[1];
            int_buff[1] = int_buff[2];
            int_buff[2] = int_buff[4];

            std::memcpy(&entry_size, int_buff, 4);
            if (numbers.find(counter) != numbers.end()) {
                char_buff = new char[entry_size];
                in.read(char_buff, entry_size);
                out.push_back(std::string(char_buff, entry_size));
                delete[] char_buff;
            } else {
                in.seekg(entry_size, std::ios::cur);
            }
            ++counter;
        }
    } else {
        std::string buff;
        while (getline(in, buff)) {
            if (numbers.find(counter) != numbers.end()) {
                out.push_back(buff);
            }
            ++counter;
            buff.resize(0);
        }
    }
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
        << "Options:\n-h, --help\tThis help page\n--test-file\tfull path to the file with test data\n" <<
        "-t\t\tType of file encoding. use -tLE if data file's entry looks like 'LE uint32 size + entry'\n" <<
        "--sm \t\t Codec will decode smaller data. Tester will show compresion ration, but not compression time\n";
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

    if (options[TINY] && options[TINY].arg != nullptr) {
        SMALL_ENTRY_NUM = std::stoi(options[TINY].arg);
    }

    std::cout << "Preparing test file. It can take some time\n";

    std::ifstream inp;
    inp.open(options[INPUT_FILE].arg);

    Codecs::HuffmanCodec codec;

    size_t sample_size = codec.sample_size(0);
    std::vector<std::string> data;
    std::vector<std::experimental::string_view> sample;
    size_t number_entries = count_entries(inp, LE_encoding);
    inp.close();
    inp.clear();
    inp.open(options[INPUT_FILE].arg);
    pickup_random_entries(inp, data, sample_size, number_entries, LE_encoding);
    for (size_t j = 0; j < data.size(); ++j) {
        sample.push_back(data[j]);
    }

    inp.close();
    inp.clear();

    std::cout << "Start Learning speed test\n";

    auto start = std::chrono::high_resolution_clock::now();
    codec.learn(sample);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    std::cout << "Learning finished in " << static_cast<long double>(duration) / 1000000000 << " seconds.\n";

    inp.open(options[INPUT_FILE].arg);
    std::string raw;
    {
        std::vector<std::string> entries;
        size_t s_len;
        if (options[TINY]) {
            s_len = read_entries(inp, entries, SMALL_ENTRY_NUM, LE_encoding);
        } else {
            s_len = read_entries(inp, entries, 0, LE_encoding);
        }
        raw.reserve(s_len + (LE_encoding ? entries.size() : 0));
        for (size_t i = 0; i < entries.size(); ++i) {
            raw.append(entries[i]);
            if (LE_encoding) {
                raw.push_back('\n');
            }
        }
    }
    inp.close();

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

    return 0;
}