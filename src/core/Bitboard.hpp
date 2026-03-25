#pragma once
#include <cstdint>
#include <bit>
#include "Types.hpp"

class Bitboard {
public:
    Bitboard() : l1{0}, l2{0}, l3{0} {}

    void set_active(uint32_t price) {
        uint64_t w1 = price >> 6;
        l1[w1] |= 1ull << (price & 63);
        l2[w1 >> 6] |= 1ull << (w1 & 63);
        l3[0] |= 1ull << ((w1 >> 6) & 63); 
    }

    void set_empty(uint32_t price) {
        uint64_t w1 = price >> 6;
        l1[w1] &= ~(1ull << (price & 63));
        if (l1[w1] == 0){l2[w1 >> 6] &= ~(1ull << (w1 & 63));}
        if (l2[(w1 >> 6)] == 0){l3[0] &= ~(1ull << ((w1 >> 6) & 63));}
    }

    uint32_t find_lowest_ask() const {
        if (l3[0] == 0) [[unlikely]] {
            return NULL_NODE;
        } else {
            int b3 = std::countr_zero(l3[0]);
            int b2 = std::countr_zero(l2[b3]);
            uint64_t w1 = (b3 * 64) + b2;
            int b1 = std::countr_zero(l1[w1]);
            return (w1 * 64) + b1;
        }
    }

    uint32_t find_highest_bid() const {
        if (l3[0] == 0) [[unlikely]] {
            return NULL_NODE;
        } else {
            int b3 = 63 - std::countl_zero(l3[0]);
            int b2 = 63 - std::countl_zero(l2[b3]);
            uint64_t w1 = (b3 * 64) + b2;
            int b1 = 63 - std::countl_zero(l1[w1]);
            return (w1 * 64) + b1;
        }
    }

private:
    uint64_t l1[1563];
    uint64_t l2[25];
    uint64_t l3[1];
};