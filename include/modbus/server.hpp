#pragma once

#include <string>
#include <array>
#include <cstdint>
#include <ostream>
#include <set>

#include <boost/asio.hpp>
#include <iostream>
#include <algorithm>
#include <functional>

#include "functions.hpp"
#include "tcp.hpp"
#include "request.hpp"
#include "response.hpp"
#include "error.hpp"
#include "impl/serialize.hpp"
#include "impl/deserialize.hpp"

namespace asio = boost::asio;
namespace ip = asio::ip;
using ip::tcp;
using asio::awaitable;
using asio::use_awaitable;
using asio::detached;

namespace modbus {

    template<typename server_handler_t>
    class connection_manager;

    template<typename server_handler_t>
    class Connection : public std::enable_shared_from_this<Connection<server_handler_t> > {
    public:
        Connection(asio::ip::tcp::socket socket,
                   std::shared_ptr<server_handler_t> handler)
                : socket_(std::move(socket)),
                  handler_(handler) {
        }

        asio::ip::tcp::socket &socket() {
            return socket_;
        }

        void start() {
            auto handler = std::bind(&Connection<server_handler_t>::handle_read,
                                     Connection<server_handler_t>::shared_from_this(), std::placeholders::_1,
                                     std::placeholders::_2);
            socket_.async_read_some(read_buffer.prepare(1024), handler);
        }

        void stop() {
            if (socket_.is_open()) {
                // socket does not always contain this information
                try {
                    std::cerr << "Connection from ip "
                              << socket_.remote_endpoint().address().to_string() << ":"
                              << socket_.remote_endpoint().port() << " Closing!" << std::endl;
                } catch (const std::system_error &e) {
                    std::cerr << "Connection closing!" << std::endl;
                }
                socket_.close();
            }
        }

        Connection() = delete;

        Connection(const Connection &) = delete;

        Connection &operator=(const Connection &) = delete;

        ~Connection() {}

    private:
        asio::ip::tcp::socket socket_;
        std::shared_ptr<server_handler_t> handler_;
        asio::streambuf read_buffer;

        void close() {
            stop();
        }

        void read_data(const std::error_code &error) {
            if (error) {
                close();
                return;
            }
            auto handler = std::bind(&Connection<server_handler_t>::handle_read,
                                     Connection<server_handler_t>::shared_from_this(), std::placeholders::_1,
                                     std::placeholders::_2);
            socket_.async_read_some(read_buffer.prepare(1024), handler);
        }

        void handle_read(const std::error_code &error, std::size_t bytes_transferred) {
            if (error) {
                close();
                return;
            } else {
                asio::streambuf write_buffer;
                auto out = std::ostreambuf_iterator<char>(&write_buffer);
                auto handler = std::bind(&Connection<server_handler_t>::read_data,
                                         Connection<server_handler_t>::shared_from_this(), std::placeholders::_1);

                read_buffer.commit(bytes_transferred); // Move the bytes from the output sequence to the input sequence
                tcp_mbap header;
                auto data = asio::buffer_cast<const uint8_t *>(read_buffer.data());
                std::error_code err;
                data = impl::deserialize(data, read_buffer.size(), header, err);
                if (err) {
                    read_buffer.consume(read_buffer.size());
                    close();
                    return;
                }
                read_buffer.consume(header.size());
                if (header.length < 2) {
                    build_error(out, header, 0, errc::illegal_function);
                } else {
                    size_t data_size = static_cast<size_t>(header.length - 1);
                    if (read_buffer.size() >= data_size) {
                        handle_data(out, header, data, data_size);
                        read_buffer.consume(data_size);
                    } else {
                        build_error(out, header, 0, errc::illegal_data_value);
                    }
                }
                asio::async_write(socket_, write_buffer, handler);
            }
        }


        void handle_data(std::ostreambuf_iterator<char> &out, tcp_mbap header, const uint8_t *data, size_t data_size) {
            // Switch can probably be avoided by templatizing requests and responses or putting them in a type list
            switch (*data) {
                case functions::read_coils:
                    handle_request<request::read_coils>(out, header, data, data_size);
                    break;
                case functions::read_discrete_inputs:;
                    handle_request<request::read_discrete_inputs>(out, header, data, data_size);
                    break;
                case functions::read_holding_registers:;
                    handle_request<request::read_holding_registers>(out, header, data, data_size);
                    break;
                case functions::read_input_registers:;
                    handle_request<request::read_input_registers>(out, header, data, data_size);
                    break;
                case functions::write_single_coil:;
                    handle_request<request::write_single_coil>(out, header, data, data_size);
                    break;
                case functions::write_single_register:;
                    handle_request<request::write_single_register>(out, header, data, data_size);
                    break;
                case functions::write_multiple_coils:;
                    handle_request<request::write_multiple_coils>(out, header, data, data_size);
                    break;
                case functions::write_multiple_registers:;
                    handle_request<request::write_multiple_registers>(out, header, data, data_size);
                    break;
                default:
                    build_error(out, header, 0, errc::illegal_function);
                    break;
            }
        }


        template<typename Request>
        void
        handle_request(std::ostreambuf_iterator<char> &out, tcp_mbap header, const uint8_t *data, size_t data_size) {
            Request req;
            std::error_code err;
            impl::deserialize(data, data_size, req, err);

            if (err) {
                build_error(out, header, 0, modbus::errc_t::illegal_data_value);
                return;
            }
            modbus::errc_t modbus_error = modbus::errc_t::no_error;
            typename Request::response resp = handler_->handle(header.unit, req, modbus_error);
            if (modbus_error) {
                build_error(out, header, 0, modbus_error);
                return;
            }
            header.length = static_cast<uint16_t>(resp.length() + 1);
            impl::serialize(out, header);
            impl::serialize(out, resp);
        }

        void build_error(std::ostreambuf_iterator<char> &out, tcp_mbap header, uint8_t function, errc::errc_t error) {
            header.length = 3;
            impl::serialize(out, header);
            impl::serialize_be8(out, function | 0X80);
            impl::serialize_be8(out, static_cast<uint8_t>(error));
        }
    };

    struct connection_state {
        connection_state(tcp::socket client) : client_(std::move(client)){
        }
        tcp::socket client_;
    };

    awaitable<void> handle_connection(tcp::socket client){
        auto state = std::make_shared<connection_state>(std::move(client));

        std::array<uint8_t, 1024> data;
        size_t n = co_await state->client_.async_read_some(asio::buffer(data), use_awaitable);

        co_await async_write(state->client_, asio::buffer(data, n), use_awaitable);
    }

    template<typename server_handler_t>
    struct server {
        server(asio::io_context &io_context, std::shared_ptr<server_handler_t> &handler, int port) :
                acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
                handler_(handler) {
        }

        ~server() {
            std::cerr << "server dtor" << std::endl;
        }

        void start() {
            co_spawn(acceptor_.get_executor(), listen(), detached);
        }

    private:
        awaitable<void> listen() {
            for (;;) {
                auto client = co_await acceptor_.async_accept(use_awaitable);
                client.set_option(asio::ip::tcp::no_delay(true));
                client.set_option(asio::socket_base::keep_alive(true));

                std::cout << "Connection opened from " << client.remote_endpoint() << "\n";
                // std::make_shared<Connection<server_handler_t>>(std::move(client), handler_)->start();

                co_spawn(acceptor_.get_executor(), handle_connection(std::move(client)), detached);

                co_await listen();
            }
        }

        asio::ip::tcp::acceptor acceptor_;
        std::shared_ptr<server_handler_t> handler_;
    };
} // namespace modbus


