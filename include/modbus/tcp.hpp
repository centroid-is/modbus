#pragma once
#include <asio.hpp>
#include <cstdint>
#include <span>
#include <utility>
#include <modbus/impl/buffer_size.h>

namespace modbus {

/// Modbus/TCP application protocol (MBAP) header.
#pragma pack(push, 1)
struct tcp_mbap {
  std::uint16_t transaction;
  std::uint16_t protocol = 0;  // 0 for modbus.
  std::uint16_t length;
  std::uint8_t unit;

  /// Header size
  static constexpr size_t size = 7;

  static auto from_bytes(res_buf_t& buffer, std::size_t offset) -> tcp_mbap {
    auto* header = std::launder(reinterpret_cast<tcp_mbap*>(buffer.data()));
    tcp_mbap ret_header;
    ret_header.transaction = ntohs(header->transaction);
    // header->protocol This is always zero
    header->protocol = 0;
    ret_header.length = ntohs(header->length);
    ret_header.unit = header->unit;
    return ret_header;
  }

  [[nodiscard]] auto to_bytes(res_buf_t& buffer, std::size_t offset) -> std::size_t {
    auto* header = std::launder(reinterpret_cast<tcp_mbap*>(&buffer[offset]));
    header->transaction = htons(transaction);
    // header->protocol This is always zero
    header->protocol = 0;
    header->length = htons(length);
    header->unit = unit;
    return tcp_mbap::size;
  }
};
#pragma pack(pop)

static_assert(sizeof(tcp_mbap) == tcp_mbap::size, "tcp_mbap has incorrect size");

/// Modbus/TCP protocol data unit (PDU).
/**
 * A Modbus/TCP PDU contains a Modbus/TCP application protocol (MBAP) header
 * and a regular Modbus application data unit (ADU).
 * The MBAP header contains additional data needed for Modbus/TCP.
 */
template <typename t>
struct tcp_pdu : tcp_mbap, t {
  /// Construct a Modbus/TCP PDU from an MBAP header and an ADU.
  [[maybe_unused]] tcp_pdu(tcp_mbap const& mbap, t const& adu) : tcp_mbap(mbap), t(adu) {}

  /// Construct a Modbus/TCP PDU from an MBAP header and an ADU.
  [[maybe_unused]] tcp_pdu(tcp_mbap const& mbap, t&& adu) : tcp_mbap(mbap), t(std::move(adu)) {}
};
}  // namespace modbus
