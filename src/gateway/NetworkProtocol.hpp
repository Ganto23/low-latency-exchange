#pragma pack(push, 1)
struct RawNetworkExecution {
    uint64_t order_id; // 8 bytes
    uint32_t price;    // 4 bytes
    uint32_t quantity; // 4 bytes
    uint8_t  status;   // 1 byte (Cast from ExecStatus)
}; // Total: 17 bytes
#pragma pack(pop)