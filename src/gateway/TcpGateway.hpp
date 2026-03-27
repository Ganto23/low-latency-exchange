#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <netinet/tcp.h>

#include "ClientSession.hpp"
#include "../concurrency/SPSCQueue.hpp"
#include "../core/Order.hpp"
#include "NetworkProtocol.hpp"

extern std::atomic<bool> g_running;

template <size_t MaxClients = 100>
class TcpGateway {
public:
    TcpGateway(SPSCQueue<OrderPayload, 65536>& in_q, SPSCQueue<ExecutionPayload, 1024>& out_q, int port) :  port(port), ingress_queue(in_q), egress_queue(out_q) {
        
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
        listen(listen_fd, 10);
        
        fcntl(listen_fd, F_SETFL, O_NONBLOCK);

        epoll_fd = epoll_create1(0);
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = listen_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
    }

    void run() {
        struct epoll_event events[MaxClients];
        std::vector<RawNetworkOrder> msg_buffer;
        msg_buffer.reserve(32);

        while (g_running) {
            int nfds = epoll_wait(epoll_fd, events, MaxClients, 0);
            
            for (int n = 0; n < nfds; ++n) {
                if (events[n].data.fd == listen_fd) {
                    handle_new_connection();
                } else {
                    int fd = events[n].data.fd;
                    auto& session = sessions.at(fd);
                    
                    msg_buffer.clear();
                    if (session.handle_read(msg_buffer)) {
                    #ifdef ENABLE_TELEMETRY
                        uint64_t batch_ingress_ts = current_nanos();
                    #endif
                        for (const auto& raw : msg_buffer) {
                            OrderPayload payload = OrderPayload::from_network(raw, fd);
                        #ifdef ENABLE_TELEMETRY
                            payload.ingress_ts = batch_ingress_ts;
                        #endif
                            ingress_queue.try_push(payload);
                        }
                    } else {
                        close_connection(fd);
                    }
                }
            }

            ExecutionPayload exec;
            while (egress_queue.try_pop(exec)) {
                RawNetworkExecution wire_msg = {
                    exec.order_id,
                    exec.price,
                    exec.quantity,
                    static_cast<uint8_t>(exec.status)
                };

                send(exec.client_fd, &wire_msg, sizeof(wire_msg), MSG_NOSIGNAL);
            }
            
            if (nfds == 0 && egress_queue.available_to_read() == 0) {
                asm volatile("yield" ::: "memory"); 
            }
        }
    }

private:
    void handle_new_connection() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd != -1) {
            fcntl(client_fd, F_SETFL, O_NONBLOCK);
            
            int one = 1;
            setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; 
            ev.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

            sessions.emplace(client_fd, ClientSession(client_fd));
            std::cout << "[Gateway] New client connected on FD: " << client_fd << std::endl;
        }
    }

    void close_connection(int fd) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        sessions.erase(fd);
        std::cout << "[Gateway] Client disconnected on FD: " << fd << std::endl;
    }

    int listen_fd;
    int epoll_fd;
    int port;
    
    SPSCQueue<OrderPayload, 65536>& ingress_queue;
    SPSCQueue<ExecutionPayload, 1024>& egress_queue;
    std::unordered_map<int, ClientSession> sessions;
};