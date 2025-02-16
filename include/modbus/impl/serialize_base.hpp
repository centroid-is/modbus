// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)
// Copyright (c) 2025, Centroid ehf (https://centroid.is)
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
#include <cstdint>
#include <vector>
#include <modbus/impl/buffer_size.h>
#include <modbus/functions.hpp>

namespace modbus::impl {

/// Convert a boolean to a uint16 Modbus representation.
inline auto bool_to_uint16(bool value) -> std::uint16_t {
  return value ? 0xff00 : 0x0000;
}

/// Serialize an uint8_t in big endian.
[[nodiscard]] inline auto serialize_be8(std::uint8_t value) -> uint8_t {
  return value;
}

/// Serialize an function_t in big endian.
[[nodiscard]] inline auto serialize_function(const function_e value, res_buf_t& buffer, const std::size_t offset) -> std::size_t {
  buffer[offset] = std::to_underlying(value);
  return offset + 1;
}

/// Serialize an uint16_t in big endian.
[[nodiscard]] inline auto serialize_be16(const std::uint16_t value) -> uint16_t {
  return htons(value);
}
// Encode uint16_t as two uint8_t
[[nodiscard]] inline auto serialize_16_array(const std::uint16_t value, res_buf_t& buffer, const std::size_t offset) -> std::size_t {
  buffer[offset] = static_cast<std::uint8_t>(value & 0xff);
  buffer[offset + 1] = static_cast<std::uint8_t>(value >> 8);
  return offset + 2;
}

/// Serialize a packed list of booleans for Modbus.
[[nodiscard]] auto serialize_bit_list(std::vector<bool> const& values, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  size_t byte_count = (values.size() + 7) / 8;

  for (std::size_t start_bit = 0; start_bit < values.size(); start_bit += 8) {
    std::uint8_t byte = 0;
    for (int sub_bit = 0; sub_bit < 8 && start_bit + sub_bit < values.size(); ++sub_bit) {
      byte |= static_cast<int>(values[start_bit + sub_bit]) << sub_bit;
    }
    buffer[offset + (start_bit / 8)] = serialize_be8(byte);
  }

  return offset + byte_count;
}

/// Serialize a vector of booleans for a Modbus request message.
[[nodiscard]] auto serialize_bits_request(std::vector<bool> const& values, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  // Serialize the bit count
  offset = serialize_16_array(serialize_be16(values.size()), buffer, offset);

  // Serialize byte count
  uint8_t byte_count = (values.size() + 7) / 8;
  buffer[offset] = serialize_be8(byte_count);
  offset += 1;

  // Serialize bits
  return serialize_bit_list(values, buffer, offset);
}

/// Serialize a vector of booleans for a Modbus response message.
[[nodiscard]] auto serialize_bits_response(std::vector<bool> const& values, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  // Serialize byte count and packed bits.
  auto byte_count = (values.size() + 7) / 8;
  buffer[offset] = serialize_be8(byte_count);
  offset += 1;

  return serialize_bit_list(values, buffer, offset);
}

/// Serialize a vector of 16 bit words for a Modbus request message.
[[nodiscard]] auto serialize_words_request(std::span<std::uint16_t> const values, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  // Serialize word count
  uint16_t word_count = serialize_be16(values.size());
  offset = serialize_16_array(word_count, buffer, offset);

  // Serialize byte_count
  buffer[offset] = serialize_be8(values.size() * 2);
  offset += 1;

  // Serialize word list
  for (auto value : values) {
    auto word = serialize_be16(value);
    offset = serialize_16_array(word, buffer, offset);
  }

  return offset;
}

/// Serialize a vector of 16 bit words for a Modbus reponse message.
[[nodiscard]] auto serialize_words_response(std::vector<std::uint16_t> const& values, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  // Serialize byte count
  auto byte_count = values.size() * 2;
  buffer[offset] = serialize_be8(byte_count);
  offset += 1;

  // Serialize values
  for (auto value : values) {
    auto word = serialize_be16(value);
    offset = serialize_16_array(word, buffer, offset);
  }
  return offset;
}

}  // namespace modbus::impl
