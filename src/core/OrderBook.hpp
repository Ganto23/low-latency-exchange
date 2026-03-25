#pragma once
#include <vector>
#include "Bitboard.hpp"
#include "FreeList.hpp"
#include "Types.hpp"

template <uint32_t MaxOrders, uint32_t MaxPrice>
class OrderBook {
public:
    OrderBook() {
        orders.resize(MaxOrders);
        id_map.resize(MaxOrders, NULL_NODE);
        bids.resize(MaxPrice);
        asks.resize(MaxPrice);
    }

    add_order()

private:
    std::vector<OrderNode> orders;
    std::vector<uint32_t> id_map;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    Bitboard bids_bits;
    Bitboard asks_bits;
    FreeList<MaxOrders> free_list;

};