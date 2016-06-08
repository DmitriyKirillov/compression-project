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
        vector<double> frequencies;

        struct queue_node {
            size_t index;
            double frequency;
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
            out.write(buff, 8);
        }

        uint64_t deserialize_64(std::istream &in) const {
            char buff[8];
            in.read(buff, 8);
            uint64_t res;
            memcpy(&res, buff, 8);
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

        void serialize_double(std::ostream &out, double val) const {
            char buff[8];
            memcpy(&buff, &val, 8);
            out.write(buff, 8);
        }

        double deserialize_double(std::istream &in) const {
            char buff[8];
            in.read(buff, 8);
            double res;
            memcpy(&res, buff, 8);
            return res;
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

        std::ostream& save(std::ostream &out) const {
            for (size_t i = 1; i < dict.size(); ++i) {
                out << static_cast<unsigned char>(dict[i].size());
                out << dict[i];
                serialize_double(out, frequencies[i]);
            }
            out << static_cast<unsigned char>(0);

            return out;
        }

        string save() const override {
            std::ostringstream out;
            save(out);

            return out.str();
        };

        void load(const string &dict) override {
            std::istringstream in(dict);
            load(in);
        };

        void load(std::istream &in) {
            auto compare = [](const queue_node &x, const queue_node &y) -> bool { return x.frequency > y.frequency; };
            std::priority_queue<queue_node, vector<queue_node>, decltype(compare)> q(compare);
            {
                size_t str_l;
                char buffer[50];
                double frequency;
                dict.resize(1);
                code_tree.resize(1);
                for (size_t j = 0; in.good(); ++j) {
                    code_tree.push_back({0, 0, true, j + 1});
                    str_l = static_cast<unsigned char>(in.get());
                    if(!str_l) {
                        break;
                    }
                    in.read(buffer, str_l);

                    dict.push_back(std::string(buffer, str_l));
                    frequency = deserialize_double(in);
                    q.push({j + 1, frequency, 1});
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

        size_t sample_size(size_t) const override {
            return sample_size();
        };

        size_t sample_size() const {
            return 30000;
        };

        void learn(const StringViewVector &samples) {
            auto compare = [](const queue_node &x, const queue_node &y) -> bool { return x.frequency > y.frequency; };
            std::priority_queue<queue_node, vector<queue_node>, decltype(compare)> q(compare);
            {
                Codecs::BOR explorer;
                explorer.learn(samples);
                std::vector<std::pair<std::string, double>> stat = explorer.move();
                dict.resize(stat.size() + 1);
                code_tree.resize(stat.size() + 1);
                frequencies.resize(stat.size() + 1);
                for (size_t j = 0; j < stat.size(); ++j) {
                    code_tree[j + 1] = {0, 0, true, j + 1};
                    dict[j + 1] = std::move(stat[j].first);
                    frequencies[j + 1] = stat[j].second;
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
