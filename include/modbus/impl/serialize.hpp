// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#pragma once

#include <modbus/request.hpp>
#include <modbus/response.hpp>

namespace modbus::impl {

[[nodiscard]] auto serialize_response(response::responses& response_variant, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  return std::visit([&buffer, offset](auto& response) mutable { return response.serialize(buffer, offset); }, response_variant);
}

[[nodiscard]] auto serialize_request(request::requests& request_variant, res_buf_t& buffer, std::size_t offset) -> std::size_t {
  return std::visit([&buffer, offset](auto& request) mutable { return request.serialize(buffer, offset); }, request_variant);
}
}  // namespace modbus::impl
