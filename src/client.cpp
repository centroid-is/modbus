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

#include <functional>
#include <system_error>

#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <iostream>

#include "modbus/client.hpp"
#include "modbus/impl/deserialize.hpp"
#include "modbus/impl/serialize.hpp"

namespace modbus {

    namespace {
    } // namespace

/// Construct a client.
    client::client(asio::io_context &io_context) : ctx(io_context), socket(io_context), no_delay_option(true),
                                                   keep_alive_option(true), _connected(false) {}

/// Set socket options
    void client::set_sock_options() {
        socket.set_option(no_delay_option);
        socket.set_option(keep_alive_option);
    }

/// Disconnect from the server.
    void client::close() {
        if (socket.is_open()) {
            // Shutdown and close socket.
            socket.shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
            socket.close();
        }
        _connected = false;
    }

/// Reset the client.
    void client::reset() {
        // Clear buffers.
        read_buffer.consume(read_buffer.size());

        // Old socket may hold now invalid file descriptor.
        socket = asio::ip::tcp::socket(io_executor());
        _connected = false;
    }

/// Called when the socket finished a read operation.
    void client::on_read(std::error_code const &error, size_t bytes_transferred) {
        if (error) {
            if (on_io_error)
                on_io_error(error);
            return;
        }

        read_buffer.commit(bytes_transferred);

        // Parse and process all complete messages in the buffer.
        while (process_message());

        // Read more data.
        auto handler = std::bind(&client::on_read, this, std::placeholders::_1, std::placeholders::_2);
        socket.async_read_some(read_buffer.prepare(1024), handler);
    }

/// Allocate a transaction in the transaction table.
    std::uint16_t client::allocate_transaction(std::uint8_t function, Handler handler) {
        std::uint16_t id = ++next_id;
        transactions.insert({id, {function, handler}});
        return id;
    }

/// Parse and process a message from the read buffer.
    bool client::process_message() {
        /// Modbus/TCP MBAP header is 7 bytes.
        if (read_buffer.size() < 7)
            return false;

        uint8_t const *data = asio::buffer_cast<uint8_t const *>(read_buffer.data());

        std::error_code error;
        tcp_mbap header;

        data = impl::deserialize(data, read_buffer.size(), header, error);

        // Handle deserialization errors in TCP MBAP.
        // Cant send an error to a specific transaction and can't continue to read from the connection.
        if (error) {
            if (on_io_error)
                on_io_error(error);
            return false;
        }

        // Ensure entire message is in buffer.
        if (read_buffer.size() < std::size_t(6 + header.length))
            return false;

        auto transaction = transactions.find(header.transaction);
        if (transaction == transactions.end()) {
            // TODO: Transaction not found. Possibly call on_io_error?
            std::cerr << "Modbus client.cpp transaction not found!" << std::endl;
            return false;
        }
        data = transaction->second.handler(data, header.length - 1, header, std::error_code());
        transactions.erase(transaction);

        // Remove read data and handled transaction.
        read_buffer.consume(6 + header.length);

        return true;
    }

} // namespace modbus
