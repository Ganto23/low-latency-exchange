#pragma once
#include "OrderBook.hpp"
#include "Order.hpp"
#include "../concurrency/SPSCQueue.hpp"
#include <vector>

class Matcher {
public:
    Matcher(SPSCQueue<ExecutionPayload, 65536>& egress) : egress_queue(egress) {
        executions.reserve(32); 
    }

    void process_payload(OrderPayload payload) {
        executions.clear();
        if (payload.action == ActionType::Cancel) {
            book.remove_order(payload.order_id, payload.is_buy, executions);
        } else {
            while (payload.quantity > 0) {
                if (payload.is_buy) {
                    uint32_t best_ask = book.get_lowest_ask();
                    if (best_ask == NULL_NODE || best_ask > payload.price) {
                        break;
                    }
                    payload.quantity = book.fill_against_price(best_ask, payload.quantity, false, payload.order_id, payload.client_fd, executions);
                } else {
                    uint32_t best_bid = book.get_highest_bid();
                    if (best_bid == NULL_NODE || best_bid < payload.price) {
                        break;
                    }
                    payload.quantity = book.fill_against_price(best_bid, payload.quantity, true, payload.order_id, payload.client_fd, executions);
                }
            }

            if (payload.quantity > 0) {
                book.add_order(payload.order_id, payload.price, payload.quantity, payload.is_buy, payload.client_fd, executions);
            }
        }

        for (const auto& exec : executions) {
            while (!egress_queue.try_push(exec)) {
                asm volatile("yield" ::: "memory");
            }
        }
    }

private:
    OrderBook<1000000, 100000> book;
    SPSCQueue<ExecutionPayload, 65536>& egress_queue;
    std::vector<ExecutionPayload> executions;
};