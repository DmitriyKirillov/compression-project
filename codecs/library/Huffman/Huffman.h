#pragma once

#include <library/common/codec.h>

namespace Codecs {

    class BinString {
    private:
        string data;
        unsigned short unused_bits;

    public:
        void push_back(bool);

        void extend(const vector<bool> &);

        void push_back(unsigned char);

        string read() const;

        BinString();

        BinString(const vector<bool> &);

        BinString(const string &);

        BinString(std::string &&);

        BinString(BinString &&);

        typename std::remove_reference<std::string>::type &&move();

        friend std::ostream &operator<<(std::ostream &, const BinString &);
    };

    class HuffmanCodec : public CodecIFace {
    public:
        struct node {
            size_t left;
            size_t right;
            bool is_leaf;
            unsigned char leaf_value;
            bool is_escape;
        };
        const unsigned MAX_CODE_L = 9;
        const unsigned BITS_PER_SYMBOL_IN_DICT = 5;
    private:
        vector<unsigned> codeLenths;
        vector<node> code_tree;
        node tree_root;
        vector<vector<bool>> precounted;
        vector<bool> escape_code;

        void InplaceSymbols(vector<node> &, size_t, const vector<unsigned char> &,
                            size_t &, size_t, size_t);

        void CountLenths(const vector<node> &, std::vector<unsigned> &, size_t, unsigned);

        void MakeCodeTree();

        void MakeCodes(const vector<node> &, size_t, vector<bool>, vector<vector<bool>> &, vector<bool> &);

        void MakeCodes();

    public:
        void encode(string &encoded, const string_view &raw) const override;

        void decode(string &raw, const string_view &encoded) const override;

        string save() const override;

        void load(const string_view &) override;

        size_t sample_size(size_t) const override {
            return 10000;
        };

        void learn(const StringViewVector &samples) override;

        void reset() override { }
    };

} //  namespace Huffman
