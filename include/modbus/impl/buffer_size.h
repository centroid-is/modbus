// Copyright (c) 2025, Centroid ehf (https://centroid.is)

#pragma once
#include <cstdint>
static constexpr size_t MODBUS_BUFFER_SIZE = 1024;
using res_buf_t = std::array<std::uint8_t, MODBUS_BUFFER_SIZE>;