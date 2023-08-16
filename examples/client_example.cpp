// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#include <iostream>
#include <thread>
#include <vector>

#include "modbus/client.hpp"

void on_io_error(std::error_code const &error) { std::cout << "Read error: " << error.message() << "\n"; }

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <hostname> <port>\n";
        return -1;
    }

    std::vector<std::thread> pool;
    asio::io_context io_context;

    std::string_view hostname = argv[1];
    std::string_view port = argv[2];

    modbus::client client{io_context};
    client.on_io_error = on_io_error;

    co_spawn(io_context, [&]() mutable -> asio::awaitable<void> {
        auto error = co_await client.connect(std::string(hostname), std::string(port), asio::use_awaitable);
        if (error) {
            std::cerr << "Error connecting: " << error.message() << std::endl;
            exit(-1);
        }
        std::cout << "Connected!" << std::endl;

        for (;;) {
            auto [error, response, _] = co_await client.read_holding_registers(0, 0, 15,
                                                                               asio::as_tuple(asio::use_awaitable));
            if (error) {
                std::cerr << "Error reading: " << error.message() << std::endl;
                exit(-1);
            }
            std::cout << "Read registers" << std::endl;
            for (std::size_t i = 0; i < response.values.size(); ++i) {
                std::cout << "\t"
                          << " " << response.values[i] << "\n";
            }
        }
        client.close();
    }, asio::detached);


    io_context.run();

}
