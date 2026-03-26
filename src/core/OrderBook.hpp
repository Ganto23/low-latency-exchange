#pragma once
#include <vector>
#include "Bitboard.hpp"
#include "FreeList.hpp"
#include "Types.hpp"
#include "Order.hpp"

template <uint32_t MaxOrders, uint32_t MaxPrice>
class OrderBook {
public:
    OrderBook() {
        orders.resize(MaxOrders);
        id_map.resize(MaxOrders, NULL_NODE);
        bids.resize(MaxPrice);
        asks.resize(MaxPrice);
    }

    void add_order(uint64_t order_id, uint32_t price, uint32_t quantity, bool is_buy, int client_fd, std::vector<ExecutionPayload>& executions) {
        uint32_t new_idx = free_list.allocate();
        orders[new_idx] = {order_id, price, quantity, NULL_NODE, NULL_NODE, client_fd}; 
        id_map[order_id] = new_idx;

        PriceLevel& level = is_buy ? bids[price] : asks[price];
        Bitboard& bits = is_buy ? bids_bits : asks_bits;

        if (level.head_idx == NULL_NODE) {
            level.head_idx = new_idx;
            level.tail_idx = new_idx;
            bits.set_active(price);
        } else {
            uint32_t old_tail = level.tail_idx;
            orders[old_tail].next_idx = new_idx;
            orders[new_idx].prev_idx = old_tail;
            level.tail_idx = new_idx;
        }

        level.total_volume += quantity;

        executions.push_back({order_id, price, quantity, client_fd, ExecStatus::Accepted});
    }

    void remove_order(uint64_t order_id, bool is_buy, std::vector<ExecutionPayload>& executions) {
        uint32_t idx_to_remove = id_map[order_id];
        if (idx_to_remove == NULL_NODE) return;
        int client_fd = orders[idx_to_remove].client_fd;

        uint32_t price = orders[idx_to_remove].price;
        uint32_t qty = orders[idx_to_remove].quantity;
        PriceLevel& level = is_buy ? bids[price] : asks[price];
        Bitboard& bits = is_buy ? bids_bits : asks_bits;

        uint32_t prev_idx = orders[idx_to_remove].prev_idx;
        uint32_t next_idx = orders[idx_to_remove].next_idx;
        if (prev_idx == NULL_NODE && next_idx == NULL_NODE) {
            level.head_idx = NULL_NODE;
            level.tail_idx = NULL_NODE;
        } else if (prev_idx == NULL_NODE) {
            level.head_idx = next_idx;
            orders[next_idx].prev_idx = NULL_NODE;
        } else if (next_idx == NULL_NODE) {
            level.tail_idx = prev_idx;
            orders[prev_idx].next_idx = NULL_NODE;
        } else {
            orders[prev_idx].next_idx = next_idx;
            orders[next_idx].prev_idx = prev_idx;
        }

        level.total_volume -= orders[idx_to_remove].quantity;

        if (level.head_idx == NULL_NODE) {
            bits.set_empty(price);
        }

        free_list.deallocate(idx_to_remove);

        id_map[order_id] = NULL_NODE;

        executions.push_back({order_id, price, qty, client_fd, ExecStatus::Canceled});
    }

    uint32_t get_lowest_ask() const {
        return asks_bits.find_lowest_ask();
    }

    uint32_t get_highest_bid() const {
        return bids_bits.find_highest_bid();
    }

    uint32_t fill_against_price(uint32_t price, uint32_t incoming_qty, bool is_buy_side_of_book, uint64_t incoming_id, int incoming_fd, std::vector<ExecutionPayload>& executions) {
        PriceLevel& level = is_buy_side_of_book ? bids[price] : asks[price];
        Bitboard& bits = is_buy_side_of_book ? bids_bits : asks_bits;

        uint32_t total_filled_at_price = 0;

        while (incoming_qty > 0 && level.head_idx != NULL_NODE) {
            uint32_t trade_idx = level.head_idx;
            uint32_t trade_qty = std::min(orders[trade_idx].quantity, incoming_qty);

            incoming_qty -= trade_qty;
            orders[trade_idx].quantity -= trade_qty;
            level.total_volume -= trade_qty;
            total_filled_at_price += trade_qty;
            int resting_fd = orders[trade_idx].client_fd;

            if (orders[trade_idx].quantity == 0) {
                executions.push_back({orders[trade_idx].order_id, price, trade_qty, resting_fd, ExecStatus::Filled});
                uint32_t dead_idx = level.head_idx;
                level.head_idx = orders[dead_idx].next_idx;
                if (level.head_idx == NULL_NODE) {
                    level.tail_idx = NULL_NODE;
                    bits.set_empty(price);
                } else {
                    orders[level.head_idx].prev_idx = NULL_NODE;
                }

                free_list.deallocate(dead_idx);

                id_map[orders[dead_idx].order_id] = NULL_NODE;
            } else {
                executions.push_back({orders[trade_idx].order_id, price, trade_qty, resting_fd, ExecStatus::Partial});
            }
        }

        if (total_filled_at_price > 0) {
            executions.push_back({
                incoming_id, 
                price, 
                total_filled_at_price, 
                incoming_fd,
                (incoming_qty == 0) ? ExecStatus::Filled : ExecStatus::Partial
            });
        }

        return incoming_qty;
    }

private:
    std::vector<OrderNode> orders;
    std::vector<uint32_t> id_map;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    Bitboard bids_bits;
    Bitboard asks_bits;
    FreeList<MaxOrders> free_list;

};