#include <library/Huffman/Huffman.h>
#include <library/common/codec.h>
#include <algorithm>
#include <bitset>
#include <functional>
#include <map>
#include <math.h>
#include <iostream>
#include <queue>

namespace Codecs {
    // Binstring class implementation
    void BinString::push_back(bool value) {
        if (unused_bits > 0) {
            if (value) {
                unsigned char add = 1;
                add <<= (unused_bits - 1);
                unsigned char val = add | static_cast<unsigned char>(data[data.size() - 1]);
                data[data.size() - 1] = static_cast<char>(val);
            }
            --unused_bits;
        } else {
            unsigned char add = (value) ? (static_cast<unsigned char>(1)) : (static_cast<unsigned char>(0));
            add <<= 7;
            data.push_back(static_cast<char>(add));
            unused_bits = 7;
        }
    }

    void BinString::extend(const vector<bool> &values) {
        data.reserve(values.size() / 8 + 1 + data.size());
        size_t pos = 0;
        while (unused_bits && pos < values.size()) {
            push_back(values[pos]);
            ++pos;
        }
        for (; pos + 8 <= values.size() && !unused_bits; pos += 8) {
            unsigned char out = 0;
            for (int i = 0; i < 8; ++i) {
                out <<= 1;
                out |= (values[pos + i]) ? (1) : (0);
            }
            data.push_back(out);
        }
        for (; pos < values.size(); ++pos) {
            push_back(values[pos]);
        }
    }

    void BinString::push_back(unsigned char value) {
        unsigned char last_b = static_cast<unsigned char>(data[data.size() - 1]);
        unsigned char add = value >> (8 - unused_bits);
        data[data.size() - 1] = static_cast<char>(last_b | add);
        data.push_back(static_cast<char>(value << unused_bits));
    }

    string BinString::read() const {
        return data;
    }

    BinString::BinString() : data(), unused_bits(0) { }

    BinString::BinString(const vector<bool> &init) {
        unused_bits = 0;
        for (bool elem : init) {
            push_back(elem);
        }
    }

    BinString::BinString(const string &init) : data(init), unused_bits(0) { }

    BinString::BinString(std::string &&s) {
        unused_bits = 0;
        data = std::move(s);
    }

    BinString::BinString(BinString &&other) {
        unused_bits = other.unused_bits;
        data = std::move(other.data);
    }

    typename std::remove_reference<std::string>::type &&BinString::move() {
        return std::move(data);
    }

    std::ostream &operator<<(std::ostream &out, const BinString &s) {
        for (char elem : s.data) {
            std::bitset<8> buffer(elem);
            out << buffer << ' ';
        }
        return out;
    }

    //Codec implementation
    //private:
    void HuffmanCodec::InplaceSymbols(vector<node> &tree, size_t v, const vector<unsigned char> &chars_list,
                                      size_t &p, size_t target_level, size_t level = 0) {
        if (p >= chars_list.size())
            return;
        if (level == target_level && tree[v].is_leaf) {
            tree[v].left = 0;
            tree[v].right = 0;
            tree[v].leaf_value = chars_list[p];
            ++p;
        } else if (level < target_level) {
            tree[v].is_leaf = false;
            InplaceSymbols(tree, tree[v].left, chars_list, p, target_level, level + 1);
            InplaceSymbols(tree, tree[v].right, chars_list, p, target_level, level + 1);
        }
    }

    void HuffmanCodec::CountLenths(const vector<node> &tree, std::vector<unsigned> &buffer, size_t p,
                                   unsigned layer = 0) {
        if (layer > MAX_CODE_L) {
            return;
        }
        if (tree[p].is_leaf) {
            buffer[tree[p].leaf_value] = layer;
        } else {
            CountLenths(tree, buffer, tree[p].left, layer + 1);
            CountLenths(tree, buffer, tree[p].right, layer + 1);
        }
    }

    void HuffmanCodec::MakeCodeTree() {
        unsigned m = 0;
        for (unsigned len : codeLenths) {
            m = std::max(m, len);
        }
        vector<vector<unsigned char>> chars_by_layer(m + 1, vector<unsigned char>());
        for (size_t i = 0; i < codeLenths.size(); ++i) {
            if (codeLenths[i]) {
                chars_by_layer[codeLenths[i]].push_back(static_cast<unsigned char>(i));
            }
        }

        size_t max_layer = chars_by_layer.size() - 1;
        if (chars_by_layer[max_layer].size() == static_cast<size_t>(std::pow(2, max_layer))) {
            ++max_layer;
        }

        code_tree.resize(static_cast<size_t>(std::pow(2, max_layer + 1)));
        for (size_t i = 1; i < code_tree.size(); ++i) {
            code_tree[i].left = 2 * i;
            code_tree[i].right = 2 * i + 1;
            code_tree[i].is_leaf = true;
            code_tree[i].is_escape = false;
            code_tree[i].leaf_value = 0;
        }

        size_t p = 1;
        for (unsigned level = 0; level < max_layer; ++level) {
            code_tree[p].is_leaf = false;
            p = code_tree[p].left;
        }
        code_tree[p].is_escape = true;

        for (size_t j = chars_by_layer.size() - 1; j > 0; --j) {
            size_t next_char_to_place = 0;
            InplaceSymbols(code_tree, 1, chars_by_layer[j], next_char_to_place, j);
        }
        tree_root = code_tree[1];
    }

    void HuffmanCodec::MakeCodes(const vector<node> &tree, size_t pos, vector<bool> prefix,
                                 vector<vector<bool>> &out, vector<bool> &escape_code) {
        if (tree[pos].is_escape) {
            escape_code = prefix;
        } else if (tree[pos].is_leaf) {
            out[tree[pos].leaf_value] = prefix;
        } else {
            vector<bool> upd = prefix;
            upd.push_back(false);
            MakeCodes(tree, tree[pos].left, upd, out, escape_code);
            *(upd.end() - 1) = true;
            MakeCodes(tree, tree[pos].right, upd, out, escape_code);
        }
    }

    void HuffmanCodec::MakeCodes() {
        precounted.resize(256);
        MakeCodes(code_tree, 1, {}, precounted, escape_code);
    }

    //public:
    void HuffmanCodec::encode(string &encoded, const string_view &raw) const {
        BinString enc;
        for (char c : raw) {
            unsigned char symbol = static_cast<unsigned char>(c);
            if (precounted[symbol].size()) {
                enc.extend(precounted[symbol]);
            } else {
                enc.extend(escape_code);
                enc.push_back(symbol);
            }
        }
        encoded = enc.move();
    }

    void HuffmanCodec::decode(string &raw, const string_view &encoded) const {
        node current_node = tree_root;
        bool code[8];
        vector<bool> escape_buffer;
        bool to_escape = false;
        for (auto It = encoded.begin(); It != encoded.end(); ++It) {
            unsigned char val = static_cast<unsigned char>(*It);
            for (int i = 7; i >= 0; --i) {
                code[i] = val % 2 != 0;
                val >>= 1;
            }
            for (int i = 0; i < 8; ++i) {
                if (to_escape) {
                    escape_buffer.push_back(code[i]);
                } else {
                    current_node = code_tree[(code[i]) ? (current_node.right) : (current_node.left)];
                    if (current_node.is_escape) {
                        to_escape = true;
                        current_node = tree_root;
                    } else if (current_node.is_leaf) {
                        raw.push_back(current_node.leaf_value);
                        current_node = tree_root;
                    }
                }

                if (escape_buffer.size() == 8) {
                    unsigned char out = 0;
                    for (int j = 0; j < 8; ++j) {
                        out <<= 1;
                        out |= escape_buffer[j];
                    }
                    raw.push_back(static_cast<char>(out));
                    to_escape = false;
                    escape_buffer.resize(0);
                }
            }

        }
    }

    string HuffmanCodec::save() const {
        BinString dict;
        vector<bool> buffer(BITS_PER_SYMBOL_IN_DICT);
        for (size_t i = 0; i < codeLenths.size(); ++i) {
            if (codeLenths[i]) {
                dict.push_back(static_cast<unsigned char>(i));
                unsigned val = codeLenths[i];
                for (unsigned j = 0; j < BITS_PER_SYMBOL_IN_DICT; ++j) {
                    buffer[j] = val % 2 != 0;
                    val >>= 1;
                }
                dict.extend(buffer);
            }
        }
        return dict.read();
    }

    void HuffmanCodec::load(const string &dict) {
        codeLenths = vector<unsigned>(256, 0);
        vector<bool> bits(8 * dict.size(), false);
        for (size_t pos = 0, i = 0; i < dict.size(); pos += 8, ++i) {
            unsigned char symbol = static_cast<unsigned char>(dict[i]);
            for (int j = 7; j >= 0; --j) {
                bits[pos + j] = symbol % 2 != 0;
                symbol >>= 1;
            }
        }

        for (int pos = 0; pos + 8 + BITS_PER_SYMBOL_IN_DICT <= bits.size(); pos += 8 + BITS_PER_SYMBOL_IN_DICT) {
            unsigned char symbol = 0;
            for (int i = 0; i < 8; ++i) {
                symbol <<= 1;
                symbol |= bits[pos + i];
            }
            unsigned lenth = 0;
            for (unsigned j = 0; j < BITS_PER_SYMBOL_IN_DICT; ++j) {
                if (bits[pos + 8 + j]) {
                    lenth += (1 << j);
                }
            }
            codeLenths[symbol] = lenth;
        }

        MakeCodeTree();
        MakeCodes();
    }

    size_t HuffmanCodec::sample_size(size_t) const {
        return 100000;
    }

    void HuffmanCodec::learn(const StringViewVector &samples) {
        vector<unsigned long long> frequencies(256, 0);
        for (auto It = samples.begin(); It != samples.end(); ++It) {
            for (auto It_s = (*It).begin(); It_s != (*It).end(); ++It_s) {
                unsigned char symbol = static_cast<unsigned char>(*It_s);
                frequencies[symbol] += 1;
            }
        }

        vector<node> tree;
        std::priority_queue<std::pair<unsigned long long, size_t>,
                vector<std::pair<unsigned long long, size_t>>,
                std::greater<std::pair<unsigned long long, size_t>>> q;
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

        codeLenths = std::vector<unsigned>(256, 0);

        CountLenths(tree, codeLenths, tree.size() - 1);
        MakeCodeTree();
        MakeCodes();
    }

    void HuffmanCodec::reset() {
        codeLenths.resize(0);
        code_tree.resize(0);
        precounted.resize(0);
        escape_code.resize(0);
    }
}