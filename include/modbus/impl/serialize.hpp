// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#pragma once

#include <modbus/request.hpp>
#include <modbus/response.hpp>

namespace modbus::impl {

[[nodiscard]] auto serialize_response(response::responses const& response_variant) -> std::vector<uint8_t> {
  return std::visit([](auto& response) { return response.serialize(); }, response_variant);
}

[[nodiscard]] auto serialize_request(request::requests const& request_variant) -> std::vector<uint8_t> {
  return std::visit([](auto& request) { return request.serialize(); }, request_variant);
}
}  // namespace modbus::impl
