// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#pragma once

#include <expected>
#include <ranges>

#include "deserialize_base.hpp"
#include "modbus/response.hpp"

namespace modbus {
namespace impl {
    std::expected<response::responses, std::error_code> response_from_function(function_t func) {
        switch (func) {
            case function_t::read_discrete_inputs:
                return response::read_discrete_inputs{};
            case function_t::read_coils:
                return response::read_coils{};
            case function_t::read_holding_registers:
                return response::read_holding_registers{};
            case function_t::read_input_registers:
                return response::read_input_registers{};
            case function_t::write_single_coil:
                return response::write_single_coil{};
            case function_t::write_single_register:
                return response::write_single_register{};
            case function_t::write_multiple_coils:
                return response::write_multiple_coils{};
            case function_t::write_multiple_registers:
                return response::write_multiple_registers{};
            case function_t::mask_write_register:
                return response::mask_write_register{};
            default:
                return std::unexpected(modbus_error(errc_t::illegal_function));
        }
    }

    /// Deserialize response. Expect a function code.
    [[nodiscard]]
    std::expected<response::responses, std::error_code>
    deserialize_response(std::ranges::range auto data, function_t const expected_function) {
        // Deserialize the function
        auto expect_function = deserialize_function(std::span(data).subspan(0), expected_function);
        if (!expect_function) return std::unexpected(expect_function.error());
        auto function = expect_function.value();

        // Fetch a response instance from the function code.
        auto expect_response = response_from_function(function);
        if (!expect_response) return std::unexpected(expect_response.error());
        auto deserialize_error = std::visit([&](auto &response) {
            return response.deserialize(data);
        }, expect_response.value());
        if (deserialize_error) return std::unexpected(deserialize_error);
        return expect_response.value();
    }

    /// Deserialize a read_coils response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::read_coils &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 1, error))
    //         return start;

    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_bits_response(start, length - 1, adu.values, error);
    //     return start;
    // }

    // /// Deserialize a read_discrete_inputs response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::read_discrete_inputs &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 1, error))
    //         return start;

    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_bits_response(start, length - 1, adu.values, error);
    //     return start;
    // }

    // /// Deserialize a read_holding_registers response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::read_holding_registers &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 1, error))
    //         return start;

    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_words_response(start, length - 1, adu.values, error);
    //     return start;
    // }

    // /// Deserialize a read_input_registers response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::read_input_registers &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 1, error))
    //         return start;

    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_words_response(start, length - 1, adu.values, error);
    //     return start;
    // }

    // /// Deserialize a write_single_coil response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::write_single_coil &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 5, error))
    //         return start;
    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_be16(start, adu.address);
    //     start = deserialize_bool(start, adu.value, error);
    //     return start;
    // }

    // /// Deserialize a write_single_register response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::write_single_register &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 5, error))
    //         return start;
    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_be16(start, adu.address);
    //     start = deserialize_be16(start, adu.value);
    //     return start;
    // }

    // /// Deserialize a write_multiple_coils response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::write_multiple_coils &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 5, error))
    //         return start;
    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_be16(start, adu.address);
    //     start = deserialize_be16(start, adu.count);
    //     return start;
    // }

    // /// Deserialize a write_multiple_registers response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::write_multiple_registers &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 5, error))
    //         return start;
    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_be16(start, adu.address);
    //     start = deserialize_be16(start, adu.count);
    //     return start;
    // }

    // /// Deserialize a mask_write_register response.
    // template <typename InputIterator>
    // InputIterator deserialize(InputIterator start, std::size_t length, response::mask_write_register &adu,
    //                           std::error_code &error) {
    //     if (!check_length(length, 7, error))
    //         return start;
    //     start = deserialize_function(start, adu.function, error);
    //     start = deserialize_be16(start, adu.address);
    //     start = deserialize_be16(start, adu.and_mask);
    //     start = deserialize_be16(start, adu.or_mask);
    //     return start;
    // }

} // namespace impl
} // namespace modbus
