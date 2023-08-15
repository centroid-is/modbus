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


//TODO: This include is only here for ntohs
// Find a better cross platform way to include this.
#include <boost/asio.hpp>

#include "modbus/error.hpp"
#include "modbus/functions.hpp"


namespace modbus {
namespace impl {

    /// Check if length is sufficient.
    [[nodiscard]] inline std::error_code check_length(std::size_t actual, std::size_t needed) {
        return actual < needed ? modbus_error(errc::message_size_mismatch) : std::error_code{};
    }

    /// Convert a uint16 Modbus boolean to a bool.
    inline std::expected<bool, std::error_code> uint16_to_bool(uint16_t value) {
        if (value == 0xff00)
            return true;
        if (value != 0x0000)
            return std::unexpected(std::error_code(errc::invalid_value, modbus_category()));
        return false;
    }

    /// Deserialize an uint8_t in big endian.
    [[nodiscard]] inline uint8_t deserialize_be8(std::span<std::byte> data) {
        assert(!data.empty());
        return static_cast<uint8_t>(data[0]);
    }

    /// Deserialize an uint16_t in big endian.
    [[nodiscard]] inline uint16_t deserialize_be16(std::span<std::byte> data) {
        assert(data.size() >= 2);
        return ntohs(*reinterpret_cast<std::uint16_t const *>(data.data()));
    }

    /// Deserialize a Modbus boolean.
    /**
	 * \return Iterator past the read sequence.
	 */
    std::expected<bool, std::error_code> deserialize_bool(std::span<std::byte> data) {
        std::uint16_t word = deserialize_be16(data);
        return uint16_to_bool(word);
    }

    /// Parse and check the function code.
    /**
	 * Does not save the parsed function code anywhere.
	 *
	 * Sets the error code to unexpected_function_code if the function code is not the expected code,
	 * but only if the error code is empty.
	 *
	 * \return The input iterator after parsing the function code.
	 */
    std::expected<function_t, std::error_code> deserialize_function(std::span<std::byte> data, function_t expected_function) {
        assert(!data.empty());
        auto function = static_cast<function_t>(data[0]);
        if (function != expected_function) return std::unexpected(modbus_error(errc::unexpected_function_code));
        return function;
    }


    /// Reads a Modbus list of bits from a byte sequence.
    /**
	 * Reads the given number of bits packed in little endian.
	 *
	 * Reads nothing if error code contains an error.
	 *
	 * \return Iterator past the read sequence.
	 */
    template <typename InputIterator>
    InputIterator deserialize_bit_list(InputIterator start, std::size_t length, std::size_t bit_count,
                                       std::vector<bool> &values, std::error_code &error) {
        if (error) return start;
        // Check available data length.
        if (!check_length(length, (bit_count + 7) / 8))
            return start;

        // Read bits.
        values.reserve(values.size() + bit_count);
        for (unsigned int start_bit = 0; start_bit < bit_count; start_bit += 8) {
            std::uint8_t byte;
            start = deserialize_be8(start, byte);

            for (unsigned int sub_bit = 0; sub_bit < 8 && start_bit + sub_bit < bit_count; ++sub_bit) {
                values.push_back(byte & 1);
                byte >>= 1;
            }
        }

        return start;
    }

    /// Read a Modbus vector of 16 bit words from a byte sequence.
    /**
	 * Reads the given number of words as 16 bit integers.
	 *
	 * Reads nothing if error code contains an error.
	 *
	 * \return Iterator past the read sequence.
	 */
    template <typename InputIterator>
    InputIterator deserialize_word_list(InputIterator start, std::size_t length, std::size_t word_count,
                                        std::vector<std::uint16_t> &values, std::error_code &error) {
        if (error) return start;
        // Check available data length.
        if (!check_length(length, word_count * 2))
            return start;

        // Read words.
        values.reserve(values.size() + word_count);
        for (unsigned int i = 0; i < word_count; ++i) {
            values.push_back(0);
            start = deserialize_be16(start, values.back());
        }

        return start;
    }

    /// Read a Modbus vector of bits from a byte sequence representing a request message.
    /**
	 * Reads bit count as 16 bit integer, byte count as 8 bit integer and finally the bits packed in little endian.
	 *
	 * Reads nothing if error code contains an error.
	 *
	 * \return Iterator past the read sequence.
	 */
    template <typename InputIterator>
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
    template <typename InputIterator>
    InputIterator deserialize_bits_response(InputIterator start, std::size_t length, std::vector<bool> &values,
                                            std::error_code &error) {
        if (error)  return start;
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
    template <typename InputIterator>
    InputIterator deserialize_words_request(InputIterator start, std::size_t length, std::vector<std::uint16_t> &values,
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
    template <typename InputIterator>
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
