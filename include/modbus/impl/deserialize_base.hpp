// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)
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

#include <bitset>
#include <cassert>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <vector>

// TODO: This include is only here for ntohs
//  Find a better cross platform way to include this.
#include <asio.hpp>

#include <modbus/error.hpp>
#include "../../../src/functions.cxx"

namespace modbus::impl {

/// Check if length is sufficient.
[[nodiscard]] inline auto check_length(std::size_t actual, std::size_t needed) -> std::error_code {
  return actual < needed ? modbus_error(errc::message_size_mismatch) : std::error_code{};
}

/// Convert a uint16 Modbus boolean to a bool.
// TODO: I can't find this behaviour in the spec.
[[nodiscard]] inline auto uint16_to_bool(uint16_t value) -> std::expected<bool, std::error_code> {
  if (value == 0xff00) {
    return true;
  }
  if (value != 0x0000) {
    return std::unexpected(std::error_code(errc::invalid_value, modbus_category()));
  }
  return false;
}

/// Deserialize an uint8_t in big endian.
[[nodiscard]] inline auto deserialize_be8(std::ranges::range auto data) -> uint8_t {
  static_assert(sizeof(typename decltype(data)::value_type) == 1);
  assert(!data.empty());
  return static_cast<uint8_t>(data[0]);
}

/// Deserialize an uint16_t in big endian.
[[nodiscard]] inline auto deserialize_be16(std::ranges::range auto data) -> uint16_t {
  static_assert(sizeof(typename decltype(data)::value_type) == 1);
  assert(data.size() >= 2);
  return ntohs(*reinterpret_cast<std::uint16_t const*>(data.data()));
}

/// Deserialize a Modbus boolean.
[[nodiscard]] inline auto deserialize_bool(std::ranges::range auto data) -> std::expected<bool, std::error_code> {
  return uint16_to_bool(deserialize_be16(data));
}

/// Parse and check the function code.
[[nodiscard]] inline auto deserialize_function(std::ranges::range auto data, function_e expected_function)
    -> std::expected<function_e, std::error_code> {
  static_assert(sizeof(typename decltype(data)::value_type) == 1);
  if (auto error = check_length(data.size(), 1)) {
    return std::unexpected(error);
  }
  if (data[0] != static_cast<uint8_t>(expected_function)) {
    return std::unexpected(modbus_error(errc::unexpected_function_code));
  }
  return static_cast<function_e>(data[0]);
}

/// Reads a Modbus list of bits from a byte sequence.
[[nodiscard]] auto deserialize_bit_list(std::ranges::range auto data, std::size_t const bit_count)
    -> std::expected<std::vector<bool>, std::error_code> {
  // Check available data length.
  size_t byte_count = (bit_count + 7) / 8;
  if (auto error = check_length(data.size(), byte_count)) {
    return std::unexpected(error);
  }

  std::vector<bool> values(bit_count);
  // Read bits.
  for (size_t start_bit = 0; start_bit < bit_count; start_bit += 8) {
    std::bitset<8> bits(deserialize_be8(std::span(data).subspan(start_bit / 8, 1)));
    for (size_t bit_index = 0; bit_index < 8 && start_bit + bit_index < bit_count; ++bit_index) {
      values[start_bit + bit_index] = bits.test(bit_index);
    }
  }
  return values;
}

/// Read a Modbus vector of 16 bit words from a byte sequence.
[[nodiscard]] auto deserialize_word_list(std::ranges::range auto data, std::size_t word_count)
    -> std::expected<std::vector<std::uint16_t>, std::error_code> {
  static_assert(sizeof(typename decltype(data)::value_type) == 1);
  // Check available data length.
  if (auto error = check_length(data.size(), word_count * 2)) {
    return std::unexpected(error);
  }

  std::vector<uint16_t> values(word_count);
  // Read words.
  for (unsigned int i = 0; i < word_count; ++i) {
    values[i] = deserialize_be16(std::span(data).subspan(i * 2, 2));
  }

  return values;
}

/// Read a Modbus vector of bits from a byte sequence representing a request message.
[[nodiscard]] auto deserialize_bits_request(std::ranges::range auto data)
    -> std::expected<std::vector<bool>, std::error_code> {
  if (auto error = check_length(data.size(), 3)) {
    return std::unexpected(error);
  }

  // Read word and byte count.
  std::uint16_t bit_count = deserialize_be16(std::span(data).subspan(0, 2));
  std::uint8_t byte_count = deserialize_be8(std::span(data).subspan(2, 1));

  // Make sure bit and byte count match.
  if (byte_count != (bit_count + 7) / 8) {
    return std::unexpected(modbus_error(errc::message_size_mismatch));
  }

  return deserialize_bit_list(std::span(data).subspan(3, data.size() - 3), bit_count);
}

/// Read a Modbus vector of bits from a byte sequence representing a response message.
[[nodiscard]] auto deserialize_bits_response(std::ranges::range auto data)
    -> std::expected<std::vector<bool>, std::error_code> {
  if (auto error = check_length(data.size(), 2)) {
    return std::unexpected(error);
  }

  // Read word and byte count.
  std::uint8_t byte_count = deserialize_be8(std::span(data).subspan(0, 1));
  return deserialize_bit_list(std::span(data).subspan(1), byte_count * 8);
}

/// Read a Modbus vector of 16 bit words from a byte sequence representing a request message.
[[nodiscard]] auto deserialize_words_request(std::ranges::range auto data)
    -> std::expected<std::vector<uint16_t>, std::error_code> {
  if (auto error = check_length(data.size(), 3)) {
    return std::unexpected(error);
  }

  // Read word and byte count.
  std::uint16_t word_count = deserialize_be16(std::span(data).subspan(0, 2));
  std::uint8_t byte_count = deserialize_be8(std::span(data).subspan(2, 1));

  // Make sure word and byte count match.
  if (byte_count != 2 * word_count) {
    return std::unexpected(modbus_error(errc::message_size_mismatch));
  }

  return deserialize_word_list(std::span(data).subspan(3, data.size() - 3), word_count);
}

/// Read a Modbus vector of 16 bit words from a byte sequence representing a response message.
[[nodiscard]] auto deserialize_words_response(std::ranges::range auto data)
    -> std::expected<std::vector<uint16_t>, std::error_code> {
  if (auto error = check_length(data.size(), 3)) {
    return std::unexpected(error);
  }

  // Read word and byte count.
  std::uint8_t byte_count = deserialize_be8(std::span(data).subspan(0, 1));
  return deserialize_word_list(std::span(data).subspan(1, data.size() - 1), byte_count / 2);
}

}  // namespace modbus::impl
