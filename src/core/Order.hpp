#pragma once
#include <cstdint>

enum class ActionType : uint8_t { New, Cancel};

enum class ExecStatus : uint8_t { Filled, Partial, Canceled };

struct OrderPayload {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    bool is_buy;
    ActionType action;
};

struct ExecutionPayload {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    ExecStatus status;
}