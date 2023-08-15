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

#pragma once

#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <expected>
#include <system_error>
#include <bitset>


//TODO: This include is only here for ntohs
// Find a better cross platform way to include this.
#include <boost/asio.hpp>

#include "modbus/error.hpp"
#include "modbus/functions.hpp"


namespace modbus {
    namespace impl {

        /// Check if length is sufficient.
        [[nodiscard]]
        inline std::error_code check_length(std::size_t actual, std::size_t needed) {
            return actual < needed ? modbus_error(errc::message_size_mismatch) : std::error_code{};
        }

        /// Convert a uint16 Modbus boolean to a bool.
        //TODO: I can't find this behaviour in the spec.
        [[nodiscard]]
        inline std::expected<bool, std::error_code> uint16_to_bool(uint16_t value) {
            if (value == 0xff00)
                return true;
            if (value != 0x0000)
                return std::unexpected(std::error_code(errc::invalid_value, modbus_category()));
            return false;
        }

        /// Deserialize an uint8_t in big endian.
        [[nodiscard]]
        inline uint8_t deserialize_be8(std::ranges::range auto data) {
            assert(!data.empty());
            return static_cast<uint8_t>(data[0]);
        }

        /// Deserialize an uint16_t in big endian.
        [[nodiscard]]
        inline uint16_t deserialize_be16(std::ranges::range auto data) {
            assert(data.size() >= 2);
            return ntohs(*reinterpret_cast<std::uint16_t const *>(data.data()));
        }

        /// Deserialize a Modbus boolean.
        [[nodiscard]]
        inline std::expected<bool, std::error_code> deserialize_bool(std::ranges::range auto data) {
            return uint16_to_bool(deserialize_be16(data));
        }

        /// Parse and check the function code.
        [[nodiscard]]
        inline std::expected<function_t, std::error_code>
        deserialize_function(std::ranges::range auto data, function_t expected_function) {
            assert(!data.empty());
            auto function = static_cast<function_t>(data[0]);
            if (function != expected_function) return std::unexpected(modbus_error(errc::unexpected_function_code));
            return function;
        }


        /// Reads a Modbus list of bits from a byte sequence.
        [[nodiscard]]
        std::expected<std::vector<bool>, std::error_code> deserialize_bit_list(std::ranges::range auto data, std::size_t const bit_count) {
            // Check available data length.
            size_t byte_count = (bit_count + 7) / 8;
            if (auto error = check_length(data.size(), byte_count))
                return std::unexpected(error);

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
        [[nodiscard]]
        inline std::expected<std::vector<std::uint16_t>, std::error_code> deserialize_word_list(std::ranges::range auto data, std::size_t word_count) {
            static_assert(sizeof(typename decltype(data)::value_type) == 1);
            // Check available data length.
            if (auto error = check_length(data.size(), word_count * 2))
                return std::unexpected(error);

            std::vector<uint16_t> values(word_count);
            // Read words.
            for (unsigned int i = 0; i < word_count; ++i) {
                values[i] = deserialize_be16(std::span(data).subspan(i * 2, 2));
            }

            return values;
        }

        /// Read a Modbus vector of bits from a byte sequence representing a request message.
        template<typename InputIterator>
        InputIterator deserialize_bits_request(InputIterator start, std::size_t length, std::vector<bool> &values,
                                               std::error_code &error) {
            if (error) return start;
            if (!check_length(length, 3))
                return start;

            // Read word and byte count.
            std::uint16_t bit_count;
            std::uint8_t byte_count;
            start = deserialize_be16(start, bit_count);
            start = deserialize_be8(start, byte_count);

            // Make sure bit and byte count match.
            if (byte_count != (bit_count + 7) / 8) {
                error = modbus_error(errc::message_size_mismatch);
                return start;
            }

            start = deserialize_bit_list(start, length - 3, bit_count, values, error);
            return start;
        }

        /// Read a Modbus vector of bits from a byte sequence representing a response message.
        /**
         * Reads byte count as 8 bit integer and finally the bits packed in little endian.
         *
         * Reads nothing if error code contains an error.
         *
         * \return Iterator past the read sequence.
         */
        template<typename InputIterator>
        InputIterator deserialize_bits_response(InputIterator start, std::size_t length, std::vector<bool> &values,
                                                std::error_code &error) {
            if (error) return start;
            if (!check_length(length, 3))
                return start;

            // Read word and byte count.
            std::uint8_t byte_count;
            start = deserialize_be8(start, byte_count);
            start = deserialize_bit_list(start, length - 1, byte_count * 8, values, error);
            return start;
        }

        /// Read a Modbus vector of 16 bit words from a byte sequence representing a request message.
        /**
         * Reads word count as 16 bit integer, byte count as 8 bit integer and finally the words as 16 bit integers.
         *
         * Reads nothing if error code contains an error.
         *
         * \return Iterator past the read sequence.
         */
        template<typename InputIterator>
        InputIterator
        deserialize_words_request(InputIterator start, std::size_t length, std::vector<std::uint16_t> &values,
                                  std::error_code &error) {
            if (error) return start;
            if (!check_length(length, 3))
                return start;

            // Read word and byte count.
            std::uint16_t word_count;
            std::uint8_t byte_count;
            start = deserialize_be16(start, word_count);
            start = deserialize_be8(start, byte_count);

            // Make sure word and byte count match.
            if (byte_count != 2 * word_count) {
                error = modbus_error(errc::message_size_mismatch);
                return start;
            }

            start = deserialize_word_list(start, length - 3, word_count, values, error);
            return start;
        }

        /// Read a Modbus vector of 16 bit words from a byte sequence representing a response message.
        /**
         * Reads byte count as 8 bit integer and finally the words as 16 bit integers.
         *
         * Reads nothing if error code contains an error.
         *
         * \return Iterator past the read sequence.
         */
        template<typename InputIterator>
        InputIterator deserialize_words_response(InputIterator start, std::size_t length,
                                                 std::vector<std::uint16_t> &values, std::error_code &error) {
            if (error) return start;
            if (!check_length(length, 3))
                return start;

            // Read word and byte count.
            std::uint8_t byte_count;
            start = deserialize_be8(start, byte_count);
            start = deserialize_word_list(start, length - 1, byte_count / 2, values, error);
            return start;
        }

    } // namespace impl
} // namespace modbus
