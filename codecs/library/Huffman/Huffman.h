#pragma once

#include <library/common/codec.h>
#include <algorithm>
#include <bitset>
#include <map>
#include <math.h>
#include <iostream>
#include <queue>

namespace Codecs {

    class BinString {
    private:
        string data;
        unsigned short unused_bits = 0;
    public:
        void push_back(bool value) {
            if (unused_bits > 0) {
                if (value) {
                    unsigned char add = std::pow(2, unused_bits - 1);
                    unsigned char val = add | static_cast<unsigned char>(data[data.size() - 1]);
                    data[data.size() - 1] = static_cast<char>(val);
                }
                --unused_bits;
            } else {
                unsigned char add = (value) ? (std::pow(2, 7)) : (0);
                data.push_back(static_cast<char>(add));
                unused_bits = 7;
            }
        }

        void push_back(const vector<bool> &values) {
            for (bool elem : values) {
                push_back(elem);
            }
        }

        void push_back(char value) {
            unsigned char val = static_cast<unsigned char>(value);
            unsigned char last_b = static_cast<unsigned char>(data[data.size() - 1]);
            unsigned char add = val >> (8 - unused_bits);
            data[data.size() - 1] = static_cast<char>(last_b | add);
            data.push_back(static_cast<char>(val << unused_bits));
        }

        template<size_t N>
        void push_back(const std::bitset<N> &values) {
            for (size_t i = 0; i < N; ++i) {
                push_back(values[i]);
            }
        }

        string read(size_t n) const {
            string val;
            size_t j = 0;
            for (; n >= 8 && j < data.size(); n /= 8, ++j) {
                val.push_back(data[j]);
            }
            if (n && j < data.size()) {
                unsigned char mask = 255 << n;
                unsigned char res = static_cast<unsigned char>(data[j]) & mask;
                val.push_back(static_cast<char>(res));
            }
            return val;
        }

        string read() const {
            return data;
        }

        BinString() : data() { }

        BinString(const vector<bool> &init) {
            for (bool elem : init) {
                push_back(elem);
            }
        }

        BinString(const string &init) : data(init) { }

        void debug_print() {
            for (char elem : data) {
                std::bitset<8> out(elem);
                std::cout << out << ' ';
            }
            std::cout << '\n';
        }
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
        const unsigned MAX_CODE_L = 7;
    private:
        std::map<char, unsigned> codeLenths;
        std::vector<node> code_tree;
        std::map<char, std::vector<bool>> precounted;
        std::vector<bool> escape_code;

        template<typename T>
        void InplaceElements(vector<node> &tree, T &It, T end, size_t p,
                             unsigned target_level, unsigned level) {
            if (It == end || level > target_level) {
                return;
            }
            if (level == target_level && tree[p].is_leaf) {
                tree[p].leaf_value = *It;
                ++It;
            } else {
                tree[p].is_leaf = false;
            }
            InplaceElements(tree, It, end, tree[p].left, target_level, level + 1);
            InplaceElements(tree, It, end, tree[p].right, target_level, level + 1);
        }

        void InplaceElements(vector<node> &tree, size_t p, unsigned target_level, unsigned level) {
            while (level < target_level) {
                p = tree[p].left;
                tree[p].is_leaf = false;
                ++level;
            }
            tree[p].is_escape = true;
        }

    public:
        void MakeCodeTree() {
            unsigned m = 0;
            std::map<unsigned, vector<char>> lenths;
            for (std::pair<char, unsigned> elem : codeLenths) {
                m = std::max(m, elem.second);
                if (lenths.find(elem.second) != lenths.end()) {
                    lenths[elem.second].push_back(elem.first);
                } else {
                    lenths[elem.second] = {elem.first};
                }
            }
            for (auto It = lenths.begin(); It != lenths.end(); ++It) {
                std::sort(It->second.begin(), It->second.end());
            }

            unsigned total_m = (std::pow(2, m) > lenths[m].size()) ? (m) : (m + 1);

            code_tree.resize(std::pow(2, total_m + 1));
            for (size_t i = 1; i < code_tree.size(); ++i) {
                code_tree[i].left = 2 * i;
                code_tree[i].right = 2 * i + 1;
                code_tree[i].is_leaf = true;
                code_tree[i].is_escape = false;
            }

            InplaceElements(code_tree, 1, total_m, 0);

            for (unsigned d = m; d > 0; --d) {
                if (lenths.find(d) != lenths.end()) {
                    auto It = lenths[d].begin();
                    InplaceElements(code_tree, It, lenths[d].end(), 1, d, 0);
                }
            }
            /*for (node elem : code_tree) {
                std::cout << elem.left << ' ' << elem.right << ' ' << elem.leaf_value << ' ' << elem.is_escape << '\n';
            }*/
        }

        void DFS(const std::vector<node> &tree, size_t v, std::map<char, std::vector<bool>> &values,
                 std::vector<bool> &escape_code, std::vector<bool> prefix) {
            if (tree[v].is_leaf) {
                values[tree[v].leaf_value] = prefix;
                return;
            }
            if (tree[v].is_escape) {
                escape_code = prefix;
                return;
            }

            std::vector<bool> left_prefix = prefix;
            left_prefix.push_back(false);
            std::vector<bool> right_prefix = prefix;
            right_prefix.push_back(true);
            DFS(tree, tree[v].left, values, escape_code, left_prefix);
            DFS(tree, tree[v].right, values, escape_code, right_prefix);
        }

        void MakeCodes() {
            if (!precounted.empty()) {
                DFS(code_tree, 1, precounted, escape_code, std::vector<bool>());
            }
        }

        void encode(string &encoded, const string_view &raw) const override {
            BinString enc;
            for (char symbol : raw) {
                if (precounted.find(symbol) != precounted.end()) {
                    enc.push_back(precounted.at(symbol));
                } else {
                    enc.push_back(escape_code);
                    enc.push_back(symbol);
                }
            }
            encoded = enc.read();
        }

        void decode(string &raw, const string_view &encoded) const override {
            raw.assign(encoded.begin(), encoded.end());
        }

        string save() const override {
            BinString dict;
            for (auto It = codeLenths.begin(); It != codeLenths.end(); ++It) {
                dict.push_back(It->first);
                std::bitset<3> len(It->second);
                dict.push_back(len);
            }
            return dict.read();
        }

        void load(const string_view &) override { }

        size_t sample_size(size_t) const override {
            return 10000;
        };

        void CountLenths(const vector<node> &v, std::map<char, unsigned> &buffer, size_t p, unsigned layer) {
            if (layer > MAX_CODE_L) {
                return;
            }
            if (v[p].is_leaf) {
                buffer[v[p].leaf_value] = layer;
            } else {
                CountLenths(v, buffer, v[p].left, layer + 1);
                CountLenths(v, buffer, v[p].right, layer + 1);
            }
        }

        void learn(const StringViewVector &samples) override {
            vector<unsigned long> frequencies(256);
            for (auto It = samples.begin(); It != samples.end(); ++It) {
                for (auto It_s = (*It).begin(); It_s != (*It).end(); ++It_s) {
                    frequencies.at(*It_s) += 1;
                }
            }


            vector<node> tree;
            std::priority_queue<std::pair<unsigned long, size_t>,
                    vector<std::pair<unsigned long, size_t>>,
                    std::greater<std::pair<unsigned long, size_t>>> q;
            for (unsigned i = 0; i < 256; ++i) {
                if (frequencies[i]) {
                    tree.push_back({0, 0, true, static_cast<unsigned char>(i), false});
                    q.push({frequencies[i], tree.size() - 1});
                }
            }

            while (q.size() > 1) {
                std::pair<unsigned long, size_t> first;
                std::pair<unsigned long, size_t> second;
                first = q.top();
                q.pop();
                second = q.top();
                q.pop();
                tree.push_back({first.second, second.second, false, 0, false});
                q.push({first.first + first.second, tree.size() - 1});
            }

            codeLenths.clear();

            CountLenths(tree, codeLenths, tree.size() - 1, 0);
            MakeCodeTree();
            MakeCodes();
        }

        void reset() override { }
    };

}
