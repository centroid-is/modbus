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
#include <expected>
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
    protected:
        /// Strand to use to prevent concurrent handler execution.
        asio::io_context &ctx;

        /// The socket to use.
        tcp::socket socket;


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
            return send_message<CompletionToken, response::read_coils>(unit, request::read_coils{address, count}, std::forward<decltype(token)>(token));
        }

        /// Read a number of discrete inputs from the connected server.
        template<typename CompletionToken>
        auto
        read_discrete_inputs(std::uint8_t unit, std::uint16_t address, std::uint16_t count, CompletionToken &&token) {
            return send_message<CompletionToken, response::read_discrete_inputs>(unit, request::read_discrete_inputs{address, count},
                                std::forward<decltype(token)>(token));
        }

        /// Read a number of holding registers from the connected server.
        template<typename CompletionToken>
        auto read_holding_registers(std::uint8_t unit, std::uint16_t address, std::uint16_t count,
                                    CompletionToken &&token) {
            return send_message<CompletionToken, response::read_holding_registers>(unit, request::read_holding_registers{address, count},
                                std::forward<decltype(token)>(token));
        }


        /// Read a number of input registers from the connected server.
        template<typename CompletionToken>
        auto
        read_input_registers(std::uint8_t unit, std::uint16_t address, std::uint16_t count, CompletionToken &&token) {
            return send_message<CompletionToken, response::read_input_registers>(unit, request::read_input_registers{address, count},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a single coil on the connected server.
        template<typename CompletionToken>
        auto write_single_coil(std::uint8_t unit, std::uint16_t address, bool value, CompletionToken &&token) {
            return send_message<CompletionToken, response::write_single_coil>(unit, request::write_single_coil{address, value}, std::forward<decltype(token)>(token));
        }

        /// Write to a single register on the connected server.
        template<typename CompletionToken>
        auto
        write_single_register(std::uint8_t unit, std::uint16_t address, std::uint16_t value, CompletionToken &&token) {
            return send_message<CompletionToken, response::write_single_register>(unit, request::write_single_register{address, value},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a number of coils on the connected server.
        template<typename CompletionToken>
        auto write_multiple_coils(std::uint8_t unit, std::uint16_t address, std::vector<bool> values,
                                  CompletionToken &&token) {
            return send_message<CompletionToken, response::write_multiple_coils>(unit, request::write_multiple_coils{address, values},
                                std::forward<decltype(token)>(token));
        }

        /// Write to a number of registers on the connected server.
        template<typename CompletionToken>
        auto
        write_multiple_registers(std::uint8_t unit, std::uint16_t address, std::vector<std::uint16_t> values,
                                 CompletionToken &&token) {
            return send_message<CompletionToken, response::write_multiple_registers>(unit, request::write_multiple_registers{address, values},
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
            return send_message<CompletionToken, response::mask_write_register>(unit, request::mask_write_register{address, and_mask, or_mask},
                                std::forward<decltype(token)>(token));
        }

    protected:
        /// Send a Modbus request to the server.
        template<typename CompletionToken, typename ResponseType>
        auto send_message(std::uint8_t unit,
                          auto const request,
                          CompletionToken &&token) {
            return async_compose<CompletionToken, void(std::expected<ResponseType, std::error_code>)>([&, request](
                    auto &self) {
                co_spawn(
                        ctx,
                        [&, self = std::move(self), request = std::move(request)]() mutable -> asio::awaitable<void> {
                            tcp_mbap request_header;
                            request_header.transaction = ++next_id;
                            request_header.protocol = 0;
                            request_header.length = request.length() + 1;
                            request_header.unit = unit;

                            auto header_encoded = request_header.to_bytes();
                            auto request_serialized = impl::serialize_request(request);

                            std::vector<asio::const_buffer> buffers{
                                    {asio::buffer(header_encoded), asio::buffer(request_serialized)}};

                            co_await asio::async_write(socket, buffers, asio::use_awaitable);

                            /// Buffer for read operations.
                            std::array<uint8_t, 7> header_buffer{};
                            // Read the response
                            auto [header_error, bytes_transferred] = co_await socket.async_read_some(
                                    asio::buffer(header_buffer, tcp_mbap::size),
                                    asio::as_tuple(asio::use_awaitable));
                            if (header_error) {
                                self.complete(std::unexpected(header_error));
                                co_return;
                            }
                            auto header = tcp_mbap::from_bytes(header_buffer);

                            // Make sure the message contains at least a function code. and a unit
                            if (header.length < 2) {
                                self.complete(std::unexpected(modbus_error(errc::message_size_mismatch)));
                                co_return;
                            }

                            std::array<uint8_t, 253> read_buffer{};
                            auto [body_error, size_of_body] = co_await socket.async_read_some(
                                    asio::buffer(read_buffer, header.length - 1), // -1 header.unit is inside the count
                                    asio::as_tuple(asio::use_awaitable));
                            if (body_error) {
                                self.complete(std::unexpected(body_error));
                                co_return;
                            }
                            if (size_of_body != header.length - 1) {
                                self.complete(std::unexpected(modbus_error(errc::message_size_mismatch)));
                                co_return;
                            }
                            // Parse and process all complete messages in the buffer.
                            auto function = static_cast<function_t>(read_buffer[0]);

                            auto response = impl::deserialize_response(read_buffer, function);

                            // Handle deserialization errors in TCP MBAP.
                            // Cant send an error to a specific transaction and can't continue to read from the connection.
                            if (!response) {
                                self.complete(std::unexpected(response.error()));
                                co_return;
                            }
                            if (std::visit([&](auto res) {
                                // Function codes 128 and above are exception responses.
                                if (static_cast<uint8_t>(res.function) >= 128) {
                                    self.complete(std::unexpected(modbus_error(
                                            header.length >= 3 ? errc_t(res.function) : errc::message_size_mismatch)
                                    ));
                                    return true;
                                }
                                return false;
                            }, response.value())) {
                                co_return;
                            }

                            auto* value = std::get_if<ResponseType>(&response.value());
                            if (value){
                                self.complete(std::move(*value));
                            } else {
                                self.complete(std::unexpected(modbus_error(errc::unexpected_function_code)));
                            }
                        }, asio::detached);
            }, token, ctx);
        }
    };


} // namespace modbus
