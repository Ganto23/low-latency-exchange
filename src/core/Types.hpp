constexpr uint32_t NULL_NODE = UINT32_MAX;

struct alignas(32) OrderNode {
    uint64_t order_id = 0;
    uint32_t price = 0;
    uint32_t quantity = 0;
    uint32_t prev_idx = NULL_NODE;
    uint32_t next_idx = NULL_NODE;
};

struct alignas(16) PriceLevel {
    uint32_t head_idx = NULL_NODE;
    uint32_t tail_idx = NULL_NODE;
    uint32_t total_volume = 0;
};