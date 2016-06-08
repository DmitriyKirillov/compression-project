#pragma once

#include <algorithm>
#include <library/common/codec.h>

#include <cstdint>
#include <forward_list>
#include <map>
#include <vector>
#include <iostream>
#include <string>

namespace Codecs {

    class BOR {
    public:
        typedef std::pair<std::string, double> dict_entry;

        struct node {
            bool is_end;
            std::map<unsigned char, size_t> next;
            uintmax_t quantity;

            node() : is_end(false), next(), quantity(0) { }

            size_t get_transition(unsigned char symbol) const {
                auto It = next.find(symbol);
                if (It != next.end()) {
                    return It->second;
                } else {
                    return 0;
                }
            }

            void set_transition(unsigned char symbol, size_t pos) {
                next[symbol] = pos;
            }

            node(uintmax_t q) : node() {
                quantity = q;
            }
        };

    private:
        const uint_fast16_t MAX_SUBSTR_L = 11;
        const double CRITERIA_EPS = 0.00000;
        //const size_t SUBSTR_TO_GET = 600;

        uintmax_t tot_lenth;
        std::vector<node> bor;
        std::vector<dict_entry> dict;

        bool criteria(uintmax_t concat_q, uintmax_t prefix_q, uintmax_t last_letter_q, size_t s_lenth) const {
            if (s_lenth == 1) {
                return true;
            }
            long double concat_p, prefix_p, letter_p;
            concat_p = concat_q;
            concat_p /= (tot_lenth - s_lenth);
            prefix_p = prefix_q;
            prefix_p /= (tot_lenth - s_lenth + 1);
            letter_p = last_letter_q;
            letter_p /= tot_lenth;
            return concat_p >= 4 * prefix_p * letter_p - CRITERIA_EPS;
        }

        void ConstructBor(const StringViewVector &sample) {
            unsigned char trans;
            bor.resize(1);
            bor[0] = node();
            //uintmax_t memory = 0;
            //uint_fast16_t node_m = sizeof node();
            for (auto It_s = sample.begin(); It_s != sample.end(); ++It_s) {
                for (auto start_pos = It_s->begin(); start_pos != It_s->end(); ++start_pos) {
                    size_t bor_pos = 0;
                    ++tot_lenth;
                    size_t next_pos;
                    for (auto current_pos = start_pos;
                         current_pos != It_s->end() && current_pos != start_pos + MAX_SUBSTR_L;
                         ++current_pos) {
                        trans = static_cast<unsigned char>(*current_pos);
                        next_pos = bor[bor_pos].get_transition(trans);
                        if (!next_pos) {
                            bor[bor_pos].set_transition(trans, bor.size());
                            next_pos = bor.size();
                            bor.push_back(node());
                        }
                        bor_pos = next_pos;
                        bor[bor_pos].quantity += 1;
                    }
                }
            }
            //std::cout << memory << '\n';
        }

        void BorCriteriaDFS(size_t bor_pos, unsigned transision = 0, size_t parent = 0, std::string prefix = "",
                            size_t layer = 0) {
            bool cool = bor_pos && criteria(bor[bor_pos].quantity,
                                            bor[parent].quantity,
                                            bor[bor[0].get_transition(transision)].quantity,
                                            layer);
            if (cool) {
                dict.push_back({prefix, static_cast<double>(bor[bor_pos].quantity) /
                                        static_cast<double>(tot_lenth - layer)});
            }
            if (cool || !bor_pos) {
                std::string new_prefix = prefix;
                new_prefix.push_back(0);
                size_t next_pos;
                for (unsigned trans = 0; trans != 256; ++trans) {
                    next_pos = bor[bor_pos].get_transition(trans);
                    if (next_pos) {
                        *new_prefix.rbegin() = static_cast<unsigned char>(trans);
                        BorCriteriaDFS(next_pos, trans, bor_pos, new_prefix, layer + 1);
                    }
                }
            }
        }

        void CorrectChars() {
            size_t next;
            for (unsigned i = 0; i != 256; ++i) {
                next = bor[0].get_transition(i);
                if (!next) {
                    bor[0].set_transition(i, bor.size());
                    bor.push_back(node(0));
                }
            }
        }

    public:

        typename std::remove_reference<std::vector<dict_entry>>::type move() const {
            return std::move(dict);
        }

        size_t sample_size(size_t) const {
            return 50000;
        };

        void learn(const StringViewVector &sample) {
            tot_lenth = 0;
            ConstructBor(sample);
            dict.reserve(bor.size() + 1);
            CorrectChars();
            BorCriteriaDFS(0);

            /*auto compare = [](const dict_entry &x, const dict_entry &y) {
                if ((x.first.size() == 1) == (y.first.size() == 1)) {
                    return x.second > y.second;
                }
                return (x.first.size() == 1) > (y.first.size() == 1);
            };
            std::sort(dict.begin(), dict.end(), compare);
            dict.resize(SUBSTR_TO_GET);
            dict.shrink_to_fit();*/
        }

        void reset() {
            bor.clear();
            dict.clear();
        }
    };

}
