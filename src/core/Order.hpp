#pragma once
#include <cstdint>

enum class ActionType : uint8_t { New, Cancel};

enum class ExecStatus : uint8_t { Filled, Partial, Accepted, Canceled };

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
    int client_fd;

    static inline OrderPayload from_network(const RawNetworkOrder& raw, int fd) {
        return {
            raw.order_id,
            raw.price,
            raw.quantity,
            raw.side == 1,
            static_cast<ActionType>(raw.action),
            fd
        };
    }
};

struct ExecutionPayload {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    int client_fd;
    ExecStatus status;
};

enum class MDEventType : uint8_t { Trade, BBOUpdate };
struct MarketDataEvent {
    MDEventType type;
    uint32_t price;
    uint32_t quantity;
    bool side; 
};

#pragma pack(push, 1)
struct MDMessage {
    uint64_t sequence_number; // Increments with every packet
    uint64_t timestamp_ns;    // When the match happened
    uint32_t price;
    uint32_t quantity;
    uint8_t  type;           // 0: Trade, 1: BBOUpdate
    uint8_t  side;           // 0: Sell, 1: Buy
};
#pragma pack(pop)