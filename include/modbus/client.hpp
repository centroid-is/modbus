// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://Skaginn3x.com)
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

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>

#include "functions.hpp"
#include "request.hpp"
#include "error.hpp"
// Make a handler that deserializes a messages and passes it to the user callback.

#include "response.hpp"
#include "tcp.hpp"
#include "impl/serialize.hpp"
#include "impl/deserialize.hpp"

namespace asio = boost::asio;
namespace ip = asio::ip;

using asio::async_compose;
using tcp = ip::tcp;

namespace modbus {


/// A connection to a Modbus server.
    class client {
    public:
        /// Callback to invoke for IO errors that cants be linked to a specific transaction.
        /**
         * Additionally the connection will be closed and every transaction callback will be called with an EOF error.
         */
        std::function<void(std::error_code const &)> on_io_error;

    protected:
        /// Low level message handler.
        using Callback = std::function<void(std::error_code const&, response::responses const&, tcp_mbap const&)>;

        /// Struct to hold transaction details.
        struct transaction_t {
            function_t function;
            Callback handler;
        };

        /// Strand to use to prevent concurrent handler execution.
        asio::io_context &ctx;

        /// The socket to use.
        tcp::socket socket;

        /// Buffer for read operations.
        std::array<uint8_t, 253> read_buffer;
        std::array<uint8_t, 7> header_buffer;

        /// Transaction table to keep track of open transactions.
        std::unordered_map<int, transaction_t> transactions;

        /// Next transaction ID.
        std::uint16_t next_id = 0;

        /// Track connected state of client.
        bool _connected{false};

        /// Socket options
        asio::ip::tcp::no_delay no_delay_option{true};
        asio::socket_base::keep_alive keep_alive_option{true};

    public:
        /// Construct a client.
        client(asio::io_context &io_context /*< The IO context to use.*/) : ctx{io_context}, socket{io_context} {}

        /// Get the IO executor used by the client.
        tcp::socket::executor_type io_executor() { return socket.get_executor(); };

        /// Connect to a server.
        template<typename CompletionToken>
        auto
        connect(const std::string &hostname, const std::string &port, CompletionToken &&token) {
            return async_compose < CompletionToken, void(std::error_code const&)>([&](auto &self) {
                co_spawn(ctx, [&, self = std::move(self)]() mutable -> asio::awaitable<void> {
                    tcp::resolver resolver{co_await asio::this_coro::executor};
                    tcp::resolver::query query{hostname, port};
                    auto [error, endpoint] = co_await resolver.async_resolve(query,
                                                                             asio::as_tuple(asio::use_awaitable));
                    if (error) {
                        self.complete(error);
                        co_return;
                    }

                    auto [connect_error, _] = co_await asio::async_connect(socket, endpoint,
                                                                           asio::as_tuple(asio::use_awaitable));
                    if (connect_error) {
                        self.complete(connect_error);
                        co_return;
                    }

                    _connected = true;

                    // Set socket options as recommended by the modbus spec.
                    socket.set_option(no_delay_option);
                    socket.set_option(keep_alive_option);

                    for (auto &transaction: transactions) {
                        transaction.second.handler(make_error_code(asio::error::operation_aborted), std::monostate{}, {});
                    }
                    transactions.clear();

                    co_spawn(ctx, process_loop(), asio::detached);

                    self.complete({});

                    co_return;
                }, asio::detached);
            }, token, ctx);
        }


        /// Disconnect from the server.
        /**
         * Any remaining transaction callbacks will be invoked with an EOF error.
         */
        void close() {
            if (socket.is_open()) {
                // Shutdown and close socket.
                socket.shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
                socket.close();
            }
            _connected = false;
        }

        /// Reset the client.
        /**
         * Should be called before re-opening a connection after a previous connection was closed.
         */
        void reset();

        /// Check if the connection to the server is open.
        /**
         * \return True if the connection to the server is open.
         */
        bool is_open() { return socket.is_open(); }

        /// Check if the client is connected.
        bool is_connected() { return is_open() && _connected; }

        /// Read a number of coils from the connected server.
        template<typename CompletionToken>
        auto read_coils(std::uint8_t unit, std::uint16_t address, std::uint16_t count, CompletionToken &&token) {
            return send_message(unit, request::read_coils{address, count}, std::forward<decltype(token)>(token));
        }

        /// Read a number of discrete inputs from the connected server.
        template<typename CompletionToken>
        auto
        read_discrete_inputs(std::uint8_t unit, std::uint16_t address, std::uint16_t count, CompletionToken &&token) {
            return send_message(unit, request::read_discrete_inputs{address, count},
                                std::forward<decltype(token)>(token));
        }

        /// Read a number of holding registers from the connected server.
        template<typename CompletionToken>
        auto read_holding_registers(std::uint8_t unit, std::uint16_t address, std::uint16_t count,
                                    CompletionToken &&token) {
            return send_message(unit, request::read_holding_registers{address, count},
                                std::forward<decltype(token)>(token));
        }


        /// Read a number of input registers from the connected server.
        template<typename CompletionToken>
        auto
        read_input_registers(std::uint8_t unit, std::uint16_t address, std::uint16_t count, CompletionToken &&token) {
            return send_message(unit, request::read_input_registers{address, count},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a single coil on the connected server.
        template<typename CompletionToken>
        auto write_single_coil(std::uint8_t unit, std::uint16_t address, bool value, CompletionToken &&token) {
            return send_message(unit, request::write_single_coil{address, value}, std::forward<decltype(token)>(token));
        }

        /// Write to a single register on the connected server.
        template<typename CompletionToken>
        auto
        write_single_register(std::uint8_t unit, std::uint16_t address, std::uint16_t value, CompletionToken &&token) {
            return send_message(unit, request::write_single_register{address, value},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a number of coils on the connected server.
        template<typename CompletionToken>
        auto write_multiple_coils(std::uint8_t unit, std::uint16_t address, std::vector<bool> values,
                                  CompletionToken &&token) {
            return send_message(unit, request::write_multiple_coils{address, values},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a number of registers on the connected server.
        template<typename CompletionToken>
        auto
        write_multiple_registers(std::uint8_t unit, std::uint16_t address, std::vector<std::uint16_t> values,
                                 CompletionToken &&token) {
            return send_message(unit, request::write_multiple_registers{address, values},
                                std::forward<decltype(token)>(token));
        }


        /// Perform a masked write to a register on the connected server.
        /**
         * Compliant servers will set the value of the register to:
         * ((old_value AND and_mask) OR (or_mask AND NOT and_MASK))
         */
        template<typename CompletionToken>
        auto
        mask_write_register(std::uint8_t unit, std::uint16_t address, std::uint16_t and_mask, std::uint16_t or_mask,
                            CompletionToken &&token) {
            return send_message(unit, request::mask_write_register{address, and_mask, or_mask},
                                std::forward<decltype(token)>(token));
        }

    protected:
        asio::awaitable<void> process_loop() {
            for (;;) {
                auto [err, bytes_transferred] = co_await socket.async_read_some(asio::buffer(header_buffer, tcp_mbap::size),
                                                                                asio::as_tuple(asio::use_awaitable));
                if (err) {
                    if (on_io_error)
                        on_io_error(err);
                    co_return;
                }

                auto header = tcp_mbap::from_bytes(header_buffer);
                auto [err2, bytes_transferred2] = co_await socket.async_read_some(asio::buffer(read_buffer, header.length - 1), // -1 header.unit is inside the count
                                                                                 asio::as_tuple(asio::use_awaitable));
                if (err2) {
                    if (on_io_error)
                        on_io_error(err2);
                    co_return;
                }
                if (bytes_transferred2 != header.length - 1) {
                    if (on_io_error)
                        on_io_error(modbus_error(errc::message_size_mismatch));
                    co_return;
                }
                // Parse and process all complete messages in the buffer.
                while (process_message(header));
            }
        }

        /// Allocate a transaction in the transaction table.
        std::uint16_t allocate_transaction(std::uint8_t function, Callback handler) {
            std::uint16_t id = ++next_id;
            transactions.insert({id, {function, std::move(handler)}});
            return id;
        }

        // Make a handler that deserializes a messages and passes it to the user callback.
        uint8_t const* handle(auto &&callback, std::uint8_t const *start, std::size_t length, tcp_mbap const &header, std::error_code error) {
            response::responses response;

            std::uint8_t const *current = start;
            std::uint8_t const *end = start + length;

            // Pass errors to callback.
            if (error) {
                callback(error, response, header);
                return current;
            }

            // Make sure the message contains at least a function code.
            if (length < 1) {
                callback(modbus_error(errc::message_size_mismatch), response, header);
                return current;
            }

            uint8_t modbus_func = *current;

            // Function codes 128 and above are exception responses.
            if (modbus_func >= 128) {
                callback(modbus_error(length >= 2 ? errc_t(start[1]) : errc::message_size_mismatch), response, header);
                return current;
            }

            // Try to deserialize the PDU.
            auto response_expected = impl::deserialize_response(std::span(current, end - current), static_cast<function_t>(modbus_func));
            if (!response_expected)
                callback(response_expected.error(), response, header);

            if (error) {
                callback(error, response, header);
                return current;
            }

            // Check response length consistency.
            // Length from the MBAP header includes the unit ID (1 byte) which is part of the MBAP header, not the response ADU.
            if (current - start != header.length - 1) {
                callback(modbus_error(errc::message_size_mismatch), response, header);
                return current;
            }

            callback(error, response, header);
            return current;
        }
        /// Parse and process a message from the read buffer.
        /**
         * \return True if a message was parsed succesfully, false if there was not enough data.
         */
        bool process_message(tcp_mbap const& header) {
            auto function = static_cast<function_t>(read_buffer[0]);
            auto ex_request = impl::deserialize_request(read_buffer, function);

            // Handle deserialization errors in TCP MBAP.
            // Cant send an error to a specific transaction and can't continue to read from the connection.
            if (!ex_request) {
                if (on_io_error)
                    on_io_error(ex_request.error());
                return false;
            }
            auto request = ex_request.value();

            // Ensure entire message is in buffer.
            if (read_buffer.size() < std::size_t(6 + header.length))
                return false;

            auto transaction = transactions.find(header.transaction);
            if (transaction == transactions.end()) {
                // TODO: Transaction not found. Possibly call on_io_error?
                std::cerr << "Modbus client.cpp transaction not found!" << std::endl;
                return false;
            }
            auto callback = transaction->second.handler;
            if (function != transaction->second.function){
                // TODO: Make this better
                throw std::runtime_error("Modbus client.cpp function mismatch!");
            }

            handle(callback, data, header.length -1, header, error);

            transactions.erase(transaction);
            // Remove read data and handled transaction.
            read_buffer.consume(6 + header.length);

            return true;
        }

        /// Send a Modbus request to the server.
        template<typename CompletionToken>
        auto send_message(std::uint8_t unit,
                          auto const &&request,
                          CompletionToken &&token) {
            using response_t = std::remove_cvref_t<decltype(request)>::response;
            return async_compose < CompletionToken, void(
                    std::error_code const, response_t const&, tcp_mbap const &)>([&](
                    auto &self) {
                co_spawn(ctx,
                         [&, self = std::move(self), request = std::move(request)]() mutable -> asio::awaitable<void> {
                             // auto handler = make_handler<response_t>(
                             //         [self = std::move(self)](std::error_code const error, response_t const &response,
                             //                                  tcp_mbap const &header) {
                             //             self.complete(error, response, header);
                             //             std::cout << "Modbus client.cpp response: " << std::endl;
                             //         });
                             tcp_mbap resp_header;
                             resp_header.transaction = allocate_transaction(request.function, [](auto&& ...args) -> uint8_t* {
                                return new uint8_t[10];
                             });
                             resp_header.protocol = 0;
                             resp_header.length = request.length() + 1;
                             resp_header.unit = unit;

                             auto header_encoded = resp_header.to_bytes();
                             asio::streambuf write_buffer{};
                             auto out = std::ostreambuf_iterator<char>(&write_buffer);
                             impl::serialize(out, request);

                             std::vector<asio::const_buffer> buffers{
                                     {asio::buffer(header_encoded), asio::buffer(write_buffer.data())}};

                             co_await asio::async_write(socket, buffers, asio::use_awaitable);
                         }, asio::detached);
            }, token, ctx);
        }
    };


} // namespace modbus
