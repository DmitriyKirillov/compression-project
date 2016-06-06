#pragma once

#include <library/common/codec.h>
#include <library/Huffman/Huffman.h>
#include <library/Bor/Bor.h>

#include <algorithm>
#include <bitset>
#include <cstring>
#include <functional>
#include <map>
#include <math.h>
#include <iostream>
#include <queue>

namespace Codecs {

    class DictHuffmanCodec : public CodecIFace {
    public:
        struct node {
            size_t left;
            size_t right;
            bool is_leaf;
            size_t dict_n;
        };

        struct search_node {
            bool is_leaf;
            size_t dict_n;
            size_t next[256];

            search_node() : is_leaf(false) {
                for (int i = 0; i < 256; ++i) {
                    next[i] = 0;
                }
            }

            search_node &operator=(const search_node &) = default;
        };
        //const unsigned MAX_CODE_L = 15;
    private:
        const double APPROX_RATIO = 0.5;

        std::vector<std::string> dict;
        vector<node> code_tree;
        node tree_root;
        vector<vector<bool>> precounted;
        vector<search_node> search_tree;

        struct queue_node {
            size_t index;
            long double frequency;
            size_t rank;
        };

        void expand_search_tree(size_t str_number) {
            size_t bor_pos = 0;
            for (char symbol : dict[str_number]) {
                unsigned char transition = static_cast<unsigned char>(symbol);
                if (!search_tree[bor_pos].next[transition]) {
                    search_tree[bor_pos].next[transition] = search_tree.size();
                    search_tree.push_back(search_node());
                }
                bor_pos = search_tree[bor_pos].next[transition];
            }
            search_tree[bor_pos].is_leaf = true;
            search_tree[bor_pos].dict_n = str_number;
        }

        void construct_search_tree() {
            search_tree.resize(1);
            search_tree[0] = search_node();
            for (size_t i = 1; i != dict.size(); ++i) {
                expand_search_tree(i);
            }
        }

        void code_tree_DFS(size_t pos, vector<bool> prefix = vector<bool>()) {
            if (code_tree[pos].is_leaf) {
                precounted[code_tree[pos].dict_n] = prefix;
            } else {
                vector<bool> new_prefix = prefix;
                new_prefix.push_back(false);
                if (code_tree[pos].left) {
                    code_tree_DFS(code_tree[pos].left, new_prefix);
                }
                *new_prefix.rbegin() = true;
                if (code_tree[pos].right) {
                    code_tree_DFS(code_tree[pos].right, new_prefix);
                }
            }
        }

        void compile_codes() {
            precounted.resize(dict.size());
            code_tree_DFS(0);
        }

        void serialize_64(std::ostream &out, uint64_t val) const {
            char buff[8];
            memcpy(&buff, &val, 8);
            out << buff;
        }

        void serialize_32(std::ostream &out, uint32_t val) const {
            char buff[4];
            memcpy(&buff, &val, 4);
            out << buff;
        }

        uint64_t deserialize_64(std::istream &in) const {
            char buff[8];
            in.read(buff, 8);
            uint64_t res;
            memcpy(&res, buff, 8);
            return res;
        }

        uint32_t deserialize_32(std::istream &in) const {
            char buff[4];
            in.read(buff, 4);
            uint64_t res;
            memcpy(&res, buff, 4);
            return res;
        }

        uint8_t deserialize_8(std::istream &in) const {
            char buff = in.get();
            uint8_t res;
            memcpy(&res, &buff, 1);
            return res;
        }

        void serialize_8(std::ostream &out, uint8_t val) const {
            char buff;
            memcpy(&buff, &val, 1);
            out << buff;
        }

        void lenth_DFS(std::vector<uint32_t> &lenths, node pos, uint32_t layer = 0) const {
            if (pos.is_leaf) {
                lenths[pos.dict_n] = layer;
            } else {
                if (pos.left) {
                    lenth_DFS(lenths, code_tree[pos.left], layer + 1);
                }
                if (pos.right) {
                    lenth_DFS(lenths, code_tree[pos.right], layer + 1);
                }
            }
        }

        void lenth_place_DFS(const std::vector<size_t> &list, size_t target_layer,
                             std::vector<node> &tree, size_t &last_pos,
                             size_t pos, size_t layer=0) {
            if (last_pos >= list.size() || layer > target_layer) {
                return;
            }
            if (layer == target_layer && tree[pos].is_leaf) {
                tree[pos].dict_n = list[last_pos];
                ++last_pos;
            } else {
                tree[pos].is_leaf = false;
                lenth_place_DFS(list, target_layer, tree, last_pos, code_tree[pos].left, layer + 1);
                lenth_place_DFS(list, target_layer, tree, last_pos, code_tree[pos].right, layer + 1);
            }
        }

    public:
        void encode(string &encoded, const string_view &raw) const override {
            BinString out;
            out.reserve_char(raw.size() * APPROX_RATIO);
            size_t pos;
            size_t last_start = 0;
            unsigned char transition;
            while (true) {
                pos = 0;
                for (size_t i = last_start; i < raw.size(); ++i) {
                    transition = static_cast<unsigned char>(raw[i]);
                    if (!search_tree[pos].next[transition]) {
                        out.extend(precounted[search_tree[pos].dict_n]);
                        pos = 0;
                        last_start = i;
                    }
                    pos = search_tree[pos].next[transition];
                }
                if (last_start < raw.size()) {
                    transition = static_cast<unsigned char>(raw[last_start]);
                    out.extend(precounted[search_tree[search_tree[0].next[transition]].dict_n]);
                    ++last_start;
                } else {
                    break;
                }
            }
            encoded = out.move();
        };

        void decode(string &raw, const string_view &encoded) const override {
            bool buffer[8];
            unsigned char symbol;
            node current = tree_root;
            for (auto It = encoded.begin(); It != encoded.end(); ++It) {
                symbol = static_cast<unsigned char>(*It);
                for (int j = 7; j >= 0; --j) {
                    buffer[j] = symbol & 1;
                    symbol >>= 1;
                }

                for (unsigned i = 0; i < 8; ++i) {
                    if (buffer[i]) {
                        current = code_tree[current.right];
                    } else {
                        current = code_tree[current.left];
                    }
                    if (current.is_leaf) {
                        raw.append(dict[current.dict_n]);
                        current = tree_root;
                    }
                }
            }
        };

        string save() const override {
            std::ostringstream out;
            serialize_64(out, dict.size());
            std::vector<uint32_t> code_lenths(dict.size());
            lenth_DFS(code_lenths, tree_root);
            for (size_t i = 1; i < code_lenths.size(); ++i) {
                serialize_8(out, dict[i].size());
                out << dict[i];
                serialize_32(out, code_lenths[i]);
            }

            return out.str();
        };

        void load(const string &dict) override {
            std::istringstream in(dict);
            load(in);
        };

        void load(std::istream &in) {
            dict.resize(deserialize_64(in));
            std::vector<uint32_t> lenths;
            uint32_t max_code = 0;
            uint8_t str_len;
            char *char_buff;
            for (size_t i = 1; i < dict.size(); ++i) {
                str_len = deserialize_8(in);
                char_buff = new char[str_len];
                in.read(char_buff, str_len);
                dict[i] = std::string(char_buff, str_len);
                delete[] char_buff;

                lenths[i] = deserialize_32(in);
                max_code = std::max(max_code, lenths[i]);
            }

            std::vector<node> tree (std::pow(2, max_code + 1) + 1, {0, 0, true, 0});
            std::vector<size_t> parents (std::pow(2, max_code + 1) + 1);
            for (size_t j = 1; j < tree.size(); ++j) {
                tree[j].left = j << 1;
                tree[j].left = (j << 1) + 1;
                parents[j] = j >> 1;
            }
            std::vector<size_t> to_add;
            size_t pos;
            for (uint32_t k = max_code; k > 0; --k) {
                to_add.resize(0);
                for (size_t i = 1; i < lenths.size(); ++i) {
                    if (lenths[i] == k) {
                        to_add.push_back(i);
                    }
                }
                pos = 0;
                lenth_place_DFS(to_add, k, tree, pos, 0);
            }
            size_t last_non_zero = tree.size() - 1;
            for (size_t j = 1; j < tree.size(); ++j) {
                if (tree[j].is_leaf && !tree[j].dict_n) {
                    for (;last_non_zero > j && tree[last_non_zero].is_leaf && !tree[last_non_zero].dict_n; --last_non_zero);
                    if (last_non_zero == j) {
                        break;
                    }
                    if (tree[parents[last_non_zero]].left == last_non_zero) {
                        tree[parents[last_non_zero]].left = j;
                    }
                    if (tree[parents[last_non_zero]].right == last_non_zero) {
                        tree[parents[last_non_zero]].right = j;
                    }
                    tree[j] = tree[last_non_zero];
                    parents[tree[last_non_zero].left] = j;
                    parents[tree[last_non_zero].right] = j;
                    tree[last_non_zero].is_leaf = true;
                    tree[last_non_zero].dict_n = 0;
                }
            }
            tree.resize(last_non_zero + 1);
            tree.shrink_to_fit();
            tree[0] = tree[1];
            code_tree = std::move(tree);

            tree_root = code_tree[0];
            compile_codes();
            construct_search_tree();
        }

        size_t sample_size(size_t) const override {
            return 100000;
        };

        size_t sample_size() const {
            return 100000;
        };

        void learn(const StringViewVector &samples) {
            auto compare = [](const queue_node &x, const queue_node &y) -> bool { return x.frequency > y.frequency; };
            std::priority_queue<queue_node, vector<queue_node>, decltype(compare)> q(compare);
            {
                Codecs::BOR explorer;
                explorer.learn(samples);
                std::vector<std::pair<std::string, long double>> stat = explorer.move();
                dict.resize(stat.size() + 1);
                code_tree.resize(stat.size() + 1);
                for (size_t j = 0; j < stat.size(); ++j) {
                    code_tree[j + 1] = {0, 0, true, j + 1};
                    dict[j + 1] = std::move(stat[j].first);
                    q.push({j + 1, stat[j].second, 1});
                }
            }

            queue_node top_one, top_second;
            while (q.size() > 1) {
                top_one = q.top();
                q.pop();
                top_second = q.top();
                q.pop();

                q.push({code_tree.size(), top_one.frequency + top_second.frequency,
                        std::max(top_one.rank, top_second.rank) + 1});
                if (top_one.rank > top_second.rank) {
                    code_tree.push_back({top_one.index, top_second.index, false, 0});
                } else {
                    code_tree.push_back({top_second.index, top_one.index, false, 0});
                }
            }
            code_tree[0] = code_tree[q.top().index];
            tree_root = code_tree[0];

            compile_codes();
            construct_search_tree();
        }

        void reset() override {
            code_tree.clear();
            search_tree.clear();
            dict.clear();
            precounted.clear();
        };
    };

} //  namespace Codecs
