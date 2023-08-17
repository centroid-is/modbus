// Copyright (c) 2017, Fizyr (https://fizyr.com)
// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#pragma once

#include "serialize_tcp.hpp"

#include "modbus/request.hpp"
#include "modbus/response.hpp"

namespace modbus {
    namespace impl {

        [[nodiscard]]
        std::vector<uint8_t> serialize_response(response::responses const &response_variant) {
            return std::visit([](auto &response) {
                return response.serialize();
            }, response_variant);
        }

        [[nodiscard]]
        std::vector<uint8_t> serialize_request(request::requests const &request_variant) {
            return std::visit([](auto &request) {
                return request.serialize();
            }, request_variant);
        }
    }
}
