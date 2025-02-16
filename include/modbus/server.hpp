// Copyright (c) 2025, Centroid ehf (https://centroid.is)

#pragma once

#include <array>
#include <expected>
#include <iostream>
#include <ranges>

#include <asio/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

#include <modbus/error.hpp>
#include <modbus/functions.hpp>
#include <modbus/impl/deserialize.hpp>
#include <modbus/impl/serialize.hpp>
#include <modbus/request.hpp>
#include <modbus/response.hpp>
#include <modbus/tcp.hpp>

namespace modbus {

namespace ip = asio::ip;
using asio::awaitable;
using asio::detached;
using asio::steady_timer;
using asio::use_awaitable;
using ip::tcp;
using std::chrono::steady_clock;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""min;
using asio::experimental::awaitable_operators::operator||;

auto handle_request(tcp_mbap const& header, res_buf_t& data, res_buf_t& buffer, std::size_t offset, auto&& handler)
    -> std::expected<std::size_t, errc_t> {
  auto req_variant = impl::deserialize_request(std::span(data), static_cast<function_e>(data[0]));
  if (!req_variant) {
    return std::unexpected(errc_t::illegal_data_value);
  }
  auto req = req_variant.value();

  auto resp = std::visit(
      [&](auto& request) -> std::expected<std::size_t, modbus::errc_t> {
        errc_t error = errc_t::no_error;
        response::responses resp = handler->handle(header.unit, request, error);
        if (error) {
          return std::unexpected(error);
        }
        return impl::serialize_response(resp, buffer, offset);
      },
      req);

  if (!resp) {
    return std::unexpected(resp.error());
  }
  return resp.value();
}

struct connection_state {
  explicit connection_state(tcp::socket&& client) : client_(std::move(client)) {}

  tcp::socket client_;
};

auto timeout(steady_clock::duration dur) -> awaitable<void> {
  steady_timer timer(co_await asio::this_coro::executor);
  timer.expires_after(dur);
  co_await timer.async_wait(use_awaitable);
}

auto build_error_buffer(tcp_mbap req_header, uint8_t function, errc::errc_t error) -> std::array<uint8_t, 9> {
  std::array<uint8_t, 9> error_buffer{};
  tcp_mbap* header = std::launder(reinterpret_cast<tcp_mbap*>(error_buffer.data()));
  header->length = htons(3);
  header->transaction = htons(req_header.transaction);
  header->unit = req_header.unit;
  error_buffer[7] = function | 0x80;
  error_buffer[8] = static_cast<uint8_t>(error);
  return error_buffer;
}

auto handle_connection(tcp::socket client, auto&& handler) -> awaitable<void> {
  auto state = std::make_shared<connection_state>(std::move(client));
  res_buf_t read_buffer{};
  res_buf_t write_buffer{};
  uint64_t rqps = 0;
  auto rqps_index = 0;
  for (;;) {
    auto result = co_await (
        state->client_.async_read_some(asio::buffer(read_buffer, tcp_mbap::size), asio::as_tuple(asio::use_awaitable)) ||
        timeout(60s));
    // Log current time for request
    if (result.index() == 1) {
      // Timeout
      std::cerr << "timeout client: " << state->client_.remote_endpoint() << " Disconnecting!" << '\n';
      break;
    }
    auto [ec, count] = std::get<0>(result);
    if (ec) {
      std::cerr << "error client: " << state->client_.remote_endpoint() << " Disconnecting!" << '\n';
      break;
    }
    if (count < tcp_mbap::size) {
      std::cerr << "packet size to small for header " << count << " " << state->client_.remote_endpoint()
                << " Disconnecting!" << std::endl;
      break;
    }
    // Deserialize the request
    auto header = tcp_mbap::from_bytes(read_buffer, 0);

    if (header.length < 2) {
      co_await async_write(state->client_, asio::buffer(build_error_buffer(header, 0, errc::illegal_function), count),
                           use_awaitable);
      continue;
    }

    // Read the request body
    auto [request_ec, request_count] = co_await state->client_.async_read_some(
        asio::buffer(read_buffer, header.length - 1), asio::as_tuple(asio::use_awaitable));
    if (request_ec) {
      std::cerr << "error client: " << state->client_.remote_endpoint() << " Disconnecting!" << '\n';
      break;
    }

    if (request_count + 1 < static_cast<size_t>(header.length)) {
      std::cerr << "packet size to small for body " << request_count << " " << state->client_.remote_endpoint()
                << " Disconnecting!" << std::endl;
      co_await async_write(state->client_, asio::buffer(build_error_buffer(header, 0, errc::illegal_data_value), count),
                           use_awaitable);
      break;
    }

    // Get an idea of the load on the server.
    auto second = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() % 10;
    if (second != rqps_index) { // We have a new second
      std::cout << "Second request_count : " << rqps << std::endl;
      rqps_index = second;
      rqps = 0;
    }
    rqps++;

    // Handle the request
    auto resp = handle_request(header, read_buffer, write_buffer, tcp_mbap::size, handler);
    if (resp) {
      header.length = resp.value() + 1;
      auto size = header.to_bytes(write_buffer, 0);
      co_await async_write(state->client_, asio::buffer(write_buffer, header.length), use_awaitable);
    } else {
      std::cerr << "error client: " << state->client_.remote_endpoint() << " error " << modbus_error(resp.error()).message()
                << '\n';
      co_await async_write(state->client_, asio::buffer(build_error_buffer(header, 0, resp.error()), count), use_awaitable);
    }
  }
  // state->client_.close();
  steady_timer wait_a_minute(co_await asio::this_coro::executor);
  wait_a_minute.expires_after(5min);
  co_await wait_a_minute.async_wait(asio::use_awaitable);
}

template <typename server_handler_t>
struct server {
  explicit server(asio::io_context& io_context, std::shared_ptr<server_handler_t>& handler, int port)
      : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), handler_(handler) {}

  void start() { co_spawn(acceptor_.get_executor(), listen(), detached); }

private:
  auto listen() -> awaitable<void> {
    for (;;) {
      auto client = co_await acceptor_.async_accept(use_awaitable);
      client.set_option(asio::ip::tcp::no_delay(true));
      client.set_option(asio::socket_base::keep_alive(true));

      co_spawn(acceptor_.get_executor(), handle_connection(std::move(client), handler_), detached);

      co_await listen();
    }
  }

  asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<server_handler_t> handler_;
};

}  // namespace modbus
