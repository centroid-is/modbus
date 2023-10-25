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
#include <cstdint>
#include <vector>

#include <modbus/error.hpp>
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
[[nodiscard]] auto serialize_function(function_e value) -> uint8_t {
  return std::to_underlying(value);
}

/// Serialize an uint16_t in big endian.
[[nodiscard]] auto serialize_be16(std::uint16_t value) -> uint16_t {
  return htons(value);
}
// Encode uint16_t as two uint8_t
[[nodiscard]] auto serialize_16_array(std::uint16_t value) -> std::array<uint8_t, 2> {
  return { static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>(value >> 8) };
}

/// Serialize a packed list of booleans for Modbus.
[[nodiscard]] auto serialize_bit_list(std::vector<bool> const& values) -> std::vector<uint8_t> {
  size_t byte_count = (values.size() + 7) / 8;
  std::vector<uint8_t> ret_value(byte_count, 0);

  for (std::size_t start_bit = 0; start_bit < values.size(); start_bit += 8) {
    std::uint8_t byte = 0;
    for (int sub_bit = 0; sub_bit < 8 && start_bit + sub_bit < values.size(); ++sub_bit) {
      byte |= static_cast<int>(values[start_bit + sub_bit]) << sub_bit;
    }
    ret_value[start_bit / 8] = serialize_be8(byte);
  }

  return ret_value;
}

/// Serialize a vector of booleans for a Modbus request message.
[[nodiscard]] auto serialize_bits_request(std::vector<bool> const& values) -> std::vector<uint8_t> {
  std::vector<uint8_t> ret_value;
  // Serialize the bit count
  auto arr = serialize_16_array(serialize_be16(values.size()));
  ret_value.insert(ret_value.end(), arr.begin(), arr.end());

  // Serialize byte count
  uint8_t byte_count = (values.size() + 7) / 8;
  ret_value.emplace_back(serialize_be8(byte_count));

  // Serialize bits
  auto bit_list = serialize_bit_list(values);
  ret_value.insert(ret_value.end(), bit_list.begin(), bit_list.end());

  return ret_value;
}

/// Serialize a vector of booleans for a Modbus response message.
[[nodiscard]] auto serialize_bits_response(std::vector<bool> const& values) -> std::vector<uint8_t> {
  std::vector<uint8_t> ret_value;
  // Serialize byte count and packed bits.
  auto byte_count = (values.size() + 7) / 8;
  ret_value.emplace_back(serialize_be8(byte_count));

  auto bit_list = serialize_bit_list(values);
  ret_value.insert(ret_value.end(), bit_list.begin(), bit_list.end());

  return ret_value;
}

/// Serialize a vector of 16 bit words for a Modbus request message.
[[nodiscard]] auto serialize_words_request(std::vector<std::uint16_t> const& values) -> std::vector<uint8_t> {
  std::vector<uint8_t> ret_value;
  // Serialize word count
  uint16_t word_count = serialize_be16(values.size());
  auto arr = serialize_16_array(word_count);
  ret_value.insert(ret_value.end(), arr.begin(), arr.end());

  // Serialize byte_count
  uint8_t byte_count = serialize_be8(values.size() * 2);
  ret_value.emplace_back(byte_count);

  // Serialize word list
  for (auto value : values) {
    auto word = serialize_be16(value);
    auto word_arr = serialize_16_array(word);
    ret_value.insert(ret_value.end(), word_arr.begin(), word_arr.end());
  }

  return ret_value;
}

/// Serialize a vector of 16 bit words for a Modbus reponse message.
[[nodiscard]] auto serialize_words_response(std::vector<std::uint16_t> const& values) -> std::vector<uint8_t> {
  std::vector<uint8_t> ret_value;
  // Serialize byte count
  auto byte_count = values.size() * 2;
  ret_value.emplace_back(serialize_be8(byte_count));

  // Serialize values
  for (auto value : values) {
    auto word = serialize_be16(value);
    auto word_arr = serialize_16_array(word);
    ret_value.insert(ret_value.end(), word_arr.begin(), word_arr.end());
  }
  return ret_value;
}

}  // namespace modbus::impl
