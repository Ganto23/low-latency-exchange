#pragma once
#include "OrderBook.hpp"
#include "Order.hpp"

class Matcher {
public:
    void process_payload(OrderPayload payload) {
        if (payload.action == ActionType::Cancel) {
            book.remove_order(payload.order_id, payload.is_buy);
            return;
        } 

        while (payload.quantity > 0) {
            if (payload.is_buy) {
                uint32_t best_ask = book.get_lowest_ask();
                if (best_ask == NULL_NODE || best_ask > payload.price) {
                    break;
                }
                payload.quantity = book.fill_against_price(best_ask, payload.quantity, false);
            } else {
                uint32_t best_bid = book.get_highest_bid();
                if (best_bid == NULL_NODE || best_bid < payload.price) {
                    break;
                }
                payload.quantity = book.fill_against_price(best_bid, payload.quantity, true);
            }
        }

        if (payload.quantity > 0) {
            book.add_order(payload.order_id, payload.price, payload.quantity, payload.is_buy);
        }
    }

private:
    OrderBook<1000000, 100000> book;

};