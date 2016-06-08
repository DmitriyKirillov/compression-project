#include <external/optionparser/optionparser.h>
#include <library/DictHuffman/DictHuffman.h>
#include <library/common/sample.h>

#include <experimental/string_view>
#include <algorithm>
#include <cstring>
#include <thread>
#include <fstream>
#include <iomanip>
#include <map>
#include <chrono>
#include <random>
#include <set>
#include <vector>
#include <stdlib.h>
#include <string>

enum optionIndex {
    UNKNOWN, HELP, INPUT_FILE, INPUT_TYPE, RECORDS, S_SIZE, SAVE
};
const option::Descriptor usage[] =
        {
                {UNKNOWN,    0, "",  "",            option::Arg::None,     ""},
                {HELP,       0, "h", "help",        option::Arg::None,     ""},
                {INPUT_FILE, 0, "",  "test-file",   option::Arg::Optional, ""},
                {INPUT_TYPE, 0, "t", "",            option::Arg::Optional, ""},
                {RECORDS,    0, "",  "records",     option::Arg::Optional, ""},
                {S_SIZE,     0, "",  "sample-size", option::Arg::Optional, ""},
                {SAVE,       0, "s", "save-test",   option::Arg::None,     ""},
                {0,          0, 0,   0,             0,                     0}
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
        while (i < length && i != n) {
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
                "--records=<number>\n\t\tCodec will encode only first <number> records from test file.\n\n"
                "--sample-size=<new size>\n\t\tTester will use <new size> entries to train codec\n\n"
                "-s, --save-test\n\t\tTest the save/load function of codec\n";
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

    size_t sample_size = 0;
    if (options[S_SIZE]) {
        if (options[S_SIZE].arg != nullptr) {
            sample_size = std::stoi(options[S_SIZE].arg);
        } else {
            std::cout << "After --sample-size you should enter new size of sample. Ex.: --sample-size=100\n";
        }
    }

    size_t records_number = 0;
    if (options[RECORDS]) {
        if (options[RECORDS].arg != nullptr) {
            records_number = std::stoi(options[RECORDS].arg);
        } else {
            std::cout << "Enter the number of records to encode. Ex.: --records=1000\n";
        }
    }

    std::cout << "Preparing test file. It can take some time\n";

    Codecs::DictHuffmanCodec codec;

    sample_size = (sample_size ? sample_size : codec.sample_size());
    std::string sample;
    std::ifstream inp;
    inp.open(options[INPUT_FILE].arg);
    std::vector<std::string> data;
    read_entries(inp, data, -1, LE_encoding);
    inp.close();
    inp.clear();
    int64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 generator(seed);

    records_number = ((records_number && records_number < data.size()) ? records_number : data.size());
    sample_size = std::min(sample_size, data.size());

    std::set<size_t> record_indexes;
    while (record_indexes.size() < sample_size) {
        record_indexes.insert(generator() % data.size());
    }
    for (auto n : record_indexes) {
        sample.append(std::string(data[n].data(), data[n].size()));
    }

    std::cout << "Start learning test\n";

    auto start = std::chrono::high_resolution_clock::now();
    codec.learn({sample});
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    std::cout << "Learning finished in " << static_cast<long double>(duration) / 1000000000 << " seconds.\n";

    uintmax_t total_encoded = 0;
    uintmax_t total_raw = 0;

    decltype(duration) enc_time[3], dec_time[3];
    enc_time[1] = enc_time[2] = dec_time[1] = dec_time[2] = 0;
    enc_time[0] = dec_time[0] = UINT32_MAX;
    double min_ratio, max_ratio;
    max_ratio = 0.0;
    min_ratio = std::numeric_limits<double>::infinity();

    std::cout << "\nStart encoding test on " << records_number << " records\n";
    std::string enc, decoded;
    for (size_t i = 0; i < records_number; ++i) {
        enc.clear();
        decoded.clear();
        start = std::chrono::high_resolution_clock::now();
        codec.encode(enc, data[i]);
        finish = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();

        enc_time[1] += duration;
        enc_time[0] = std::min(enc_time[0], duration);
        enc_time[2] = std::max(enc_time[2], duration);

        start = std::chrono::high_resolution_clock::now();
        codec.decode(decoded, enc);
        finish = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();

        dec_time[1] += duration;
        dec_time[0] = std::min(dec_time[0], duration);
        dec_time[2] = std::max(dec_time[2], duration);

        if (decoded != data[i]) {
            std::cout << "Encoded/Decodec incorrectly on the " << i << " record in file "
            << options[INPUT_FILE].arg << '\n';
            return 0;
        }

        total_encoded += enc.size();
        total_raw += data[i].size();
        min_ratio = std::min(min_ratio, (double) 1.0 - (double) enc.size() / (double) data[i].size());
        max_ratio = std::max(min_ratio, (double) 1.0 - (double) enc.size() / (double) data[i].size());
    }
    std::string saved = codec.save();
    std::cout << "\nCompression ratio:\nMin: " << min_ratio
    << "\tMax: " << max_ratio
    << "\tAverage: "
    << (double) 1.0 - (double) total_encoded / (double) total_raw
    << "\nOn whole file (including dictionary):\t"
    << (double) 1 - (double) (total_encoded + saved.size()) / (double) total_raw
    << "\n\nCompression time:\nMin: " << (long double) enc_time[0] / 1000000
    << "\tMax: " << (long double) enc_time[2] / 1000000
    << "\tAverage: " << (long double) enc_time[1] / ((uint64_t) 1000000 * records_number)
    << "\nTime spent on whole file encoding: " << (double) enc_time[1] / 1000000 << " miliseconds\n"
    << "\nDecompression time:\nMin: " << (long double) dec_time[0] / 1000000
    << "\tMax: " << (long double) dec_time[2] / 1000000
    << "\tAverage: " << (long double) dec_time[1] / ((uint64_t) 1000000 * records_number)
    << "\nTime spent on whole file encoding: " << (double) dec_time[1] / 1000000 << " miliseconds\n";

    if (options[SAVE]) {
        size_t to_test = std::min((size_t)3000, data.size());

        std::cout << "\nStart serializing test:\nDictionary size is "
        << std::fixed << std::setprecision(2) << (double)saved.size() / ((uint32_t)1 << 20) << " MB\n";
        std::set<size_t> test_numbers;
        while (test_numbers.size() < to_test) {
            test_numbers.insert(generator() % data.size());
        }
        std::vector<std::string> previous(to_test);
        size_t i = 0;
        for (auto It = test_numbers.begin(); It != test_numbers.end(); ++i, ++It) {
            codec.encode(previous[i], data[*It]);
        }

        codec.load(saved);
        i = 0;
        std::string plain;
        bool good = true;
        for (auto It = test_numbers.begin(); It != test_numbers.end(); ++i, ++It) {
            plain.clear();
            codec.decode(plain, previous[i]);
            if (plain != data[*It]) {
                good = false;
            }
        }
        std::cout << "Save/Load process finished " << (good ? "correctly" : "incorrectly") << '\n';
    }


    return 0;
}