// Copyright (c) 2018, J.R. Versteegh (https://www.orca-st.com)
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
// DISCLAIMED. IN NO EVENT SHALL J.R. Versteegh BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MODBUS_SERVER_H_
#define MODBUS_SERVER_H_
#pragma once

#include <string>
#include <array>
#include <cstdint>
#include <ostream>
#include <set>

#include <asio.hpp>
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



namespace modbus {

template <typename ServerHandler>
class ConnectionManager;

template <typename ServerHandler>
class Connection : public std::enable_shared_from_this<Connection<ServerHandler> > {
  public:
    Connection(asio::ip::tcp::socket socket, ConnectionManager<ServerHandler>& mgr, std::shared_ptr<ServerHandler> handler)
        : socket_(std::move(socket)),
          handler_(handler),
          _mgr(mgr){
    }

  asio::ip::tcp::socket& socket() {
    return socket_;
  }

  void start(){
      _mgr.join(Connection<ServerHandler>::shared_from_this());
      auto handler = std::bind(&Connection<ServerHandler>::handle_read, Connection<ServerHandler>::shared_from_this(), std::placeholders::_1, std::placeholders::_2);
      socket_.async_read_some(read_buffer.prepare(1024), handler);
  }

  void stop(){
      if (socket_.is_open()) {
          // socket does not always contain this information
          try{
            std::cerr << "Connection from ip " 
                    << socket_.remote_endpoint().address().to_string() << ":"
                    << socket_.remote_endpoint().port() << " Closing!" << std::endl;
          } catch(const std::system_error& e){
            std::cerr << "Connection closing!" << std::endl;
          }
          socket_.close();
      }
  }

  Connection() = delete;
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  ~Connection() {}
private:
    asio::ip::tcp::socket socket_;
    ConnectionManager<ServerHandler>& _mgr;
    std::shared_ptr<ServerHandler> handler_;
    asio::streambuf read_buffer;

    void close() {
        stop();
        _mgr.leave(Connection<ServerHandler>::shared_from_this());
    }
    void read_data(const std::error_code& error) {
        if ( error ) {
            close();
            return;
      }
      auto handler = std::bind(&Connection<ServerHandler>::handle_read, Connection<ServerHandler>::shared_from_this(), std::placeholders::_1, std::placeholders::_2);
      socket_.async_read_some(read_buffer.prepare(1024), handler);
    }
    void handle_read(const std::error_code& error, std::size_t bytes_transferred) {
        if ( error ) {
            close();
            return;
        } else {
            asio::streambuf write_buffer;
            auto out = std::ostreambuf_iterator<char>(&write_buffer);
            auto handler = std::bind(&Connection<ServerHandler>::read_data, Connection<ServerHandler>::shared_from_this(), std::placeholders::_1);

            read_buffer.commit(bytes_transferred); // Move the bytes from the output sequence to the input sequence
            tcp_mbap header;
            auto data = asio::buffer_cast<const uint8_t*>(read_buffer.data());
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
                }
                else {
                    build_error(out, header, 0, errc::illegal_data_value);
                }
            }
            asio::async_write(socket_, write_buffer, handler);
        }
  }


  void handle_data(std::ostreambuf_iterator<char>& out, tcp_mbap header, const uint8_t* data, size_t data_size) {
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


  template <typename Request>
  void handle_request(std::ostreambuf_iterator<char>& out, tcp_mbap header, const uint8_t* data, size_t data_size) {
    Request req;
    std::error_code err;
    impl::deserialize(data, data_size, req, err);

    if ( err ) {
        build_error(out, header, 0, modbus::errc_t::illegal_data_value);
        return;
    }
    modbus::errc_t modbus_error = modbus::errc_t::no_error;
    typename Request::response resp = handler_->handle(header.unit, req, modbus_error);
    if ( modbus_error ) {
        build_error(out, header, 0, modbus_error);
        return;
    }
    header.length = static_cast<uint16_t>(resp.length() + 1);
    impl::serialize(out, header);
    impl::serialize(out, resp);
  }
  void build_error(std::ostreambuf_iterator<char>& out, tcp_mbap header, uint8_t function, errc::errc_t error){
    header.length = 3;
    impl::serialize(out, header);
    impl::serialize_be8(out, function | 0X80);
    impl::serialize_be8(out, static_cast<uint8_t>(error));
  }
};

template <typename ServerHandler>
class ConnectionManager{
  public:
    ConnectionManager(){}
    void join(std::shared_ptr<Connection<ServerHandler>> ptr){
        _con.insert(ptr);
    }
    void leave(std::shared_ptr<Connection<ServerHandler>> ptr){
        _con.erase(ptr);
    }
    void stop(){
        for(auto&& connection : _con ){
            connection->stop();
        }
    }
  private:
    std::set<std::shared_ptr<Connection<ServerHandler>>> _con;
};

// A Modbus server base class
template <typename ServerHandler>
struct Server: public std::enable_shared_from_this<Server<ServerHandler> > {
    Server(asio::io_context& io_context, std::shared_ptr<ServerHandler>& handler, int port):
        _acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        _handler(handler),
        _mgr() {
        start_accept();
    }
    ~Server(){
        std::cerr << "server dtor" << std::endl;
    }
  void stop() {
    _acceptor.cancel();
    _acceptor.close();
    _mgr.stop();
  }
private:
  void start_accept() {
      _acceptor.async_accept(
          [this](const std::error_code& ec, asio::ip::tcp::socket socket){
            if(!ec){
                std::cerr << "accepted new connection with ip : " << socket.remote_endpoint().address().to_string() <<
                                                             ":" << socket.remote_endpoint().port() << std::endl;
                // Set socket options recomended by modbus implementation guide.                                             

                socket.set_option(asio::ip::tcp::no_delay(true));
                socket.set_option(asio::socket_base::keep_alive(true));
                std::make_shared<Connection<ServerHandler>>(std::move(socket), _mgr, _handler)->start();

                start_accept();
            } else {
                std::cerr << "Acceptor error : " << ec.message() << std::endl;
            }
          });
  }
  asio::ip::tcp::acceptor _acceptor;
  std::shared_ptr<ServerHandler> _handler;
  ConnectionManager<ServerHandler> _mgr;
};


struct Default_handler {
    Default_handler()
        : registers_(0x20000), coils_(0x20000) {}

    modbus::response::read_coils handle(uint8_t, const modbus::request::read_coils& req, errc_t& modbus_error) {
        modbus::response::read_coils resp;
        resp.values.insert(
            resp.values.end(),
            coils_.cbegin() + req.address,
            coils_.cbegin() + req.address + req.count
        );
        return resp;
    }

    modbus::response::read_discrete_inputs handle(uint8_t, const modbus::request::read_discrete_inputs& req, errc_t& modbus_error) {
        modbus::response::read_discrete_inputs resp;
        resp.values.resize(req.count);
        return resp;
    }

    modbus::response::read_holding_registers handle(uint8_t, const modbus::request::read_holding_registers& req, errc_t& modbus_error) {
        modbus::response::read_holding_registers resp;
        resp.values.insert(
            resp.values.end(),
            registers_.cbegin() + req.address,
            registers_.cbegin() + req.address + req.count
        );
        return resp;
    }

    modbus::response::read_input_registers handle(uint8_t, const modbus::request::read_input_registers& req, errc_t& modbus_error) {
        modbus::response::read_input_registers resp;
        resp.values.resize(req.count);
        return resp;
    }

    modbus::response::write_single_coil handle(uint8_t, const modbus::request::write_single_coil& req, errc_t& modbus_error) {
        modbus::response::write_single_coil resp;
        coils_[req.address] = req.value;
        resp.address = req.address;
        resp.value = req.value;
        return resp;
    }

    modbus::response::write_single_register handle(uint8_t, const modbus::request::write_single_register& req, errc_t& modbus_error) {
        modbus::response::write_single_register resp;
        registers_[req.address] = req.value;
        resp.address = req.address;
        resp.value = req.value;
        return resp;
    }

    modbus::response::write_multiple_coils handle(uint8_t, const modbus::request::write_multiple_coils& req, errc_t& modbus_error) {
        modbus::response::write_multiple_coils resp;
        resp.address = req.address;
        resp.count = 0;
        auto iit = req.values.begin();
        auto oit = coils_.begin() + req.address;
        while (iit < req.values.end() && oit < coils_.end()) {
            *oit++ = *iit++;
            ++resp.count;
        }
        return resp;
    }

    modbus::response::write_multiple_registers handle(uint8_t, const modbus::request::write_multiple_registers& req, errc_t& modbus_error) {
        modbus::response::write_multiple_registers resp;
        resp.address = req.address;
        resp.count = 0;
        auto iit = req.values.begin();
        auto oit = registers_.begin() + req.address;
        while (iit < req.values.end() && oit < registers_.end()) {
            *oit++ = *iit++;
            ++resp.count;
        }
        return resp;
    }
  private:
    std::vector<std::uint16_t> registers_;
    std::vector<bool> coils_;
};

} // namespace modbus
#endif
