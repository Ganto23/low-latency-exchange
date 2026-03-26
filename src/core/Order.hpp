#pragma once
#include <cstdint>

enum class ActionType : uint8_t { New, Cancel};

enum class ExecStatus : uint8_t { Filled, Partial, Canceled };

#pragma pack(push, 1)
struct RawNetworkOrder {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    uint8_t  side;
    uint8_t  action;
    uint32_t checksum;
};
#pragma pack(pop)

struct OrderPayload {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    bool is_buy;
    ActionType action;

    static inline OrderPayload from_network(const RawNetworkOrder& raw) {
        return {
            raw.order_id,
            raw.price,
            raw.quantity,
            raw.side == 1,
            static_cast<ActionType>(raw.action)
        };
    }
};

struct ExecutionPayload {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    ExecStatus status;
};