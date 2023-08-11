// Copyright (c) 2017, Fizyr (https://fizyr.com)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the copyright holder(s) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
