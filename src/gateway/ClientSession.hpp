#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include "../core/Order.hpp"

class ClientSession {
public:
    explicit ClientSession(int fd) : socket_fd(fd), write_idx(0) {}

    bool handle_read(std::vector<RawNetworkOrder>& out_messages) {
        ssize_t bytes_received = recv(socket_fd, buffer.data() + write_idx, buffer.size() - write_idx, 0);

        if (bytes_received <= 0) return false;

        write_idx += bytes_received;

        size_t processed_idx = 0;
        const size_t MSG_SIZE = sizeof(RawNetworkOrder);

        while (write_idx - processed_idx >= MSG_SIZE) {
            RawNetworkOrder msg;
            std::memcpy(&msg, buffer.data() + processed_idx, MSG_SIZE);
            
            // TODO: Optional Checksum validation here
            out_messages.push_back(msg);
            
            processed_idx += MSG_SIZE;
        }

        if (processed_idx > 0) {
            size_t remaining = write_idx - processed_idx;
            if (remaining > 0) {
                std::memmove(buffer.data(), buffer.data() + processed_idx, remaining);
            }
            write_idx = remaining;
        }

        return !out_messages.empty();
    }

    int get_fd() const { return socket_fd; }

private:
    int socket_fd;
    size_t write_idx;
    std::array<uint8_t, 1024> buffer; 
};