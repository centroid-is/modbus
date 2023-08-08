#pragma once

#include "server.hpp"
#include "error.hpp"

struct default_handler {
    default_handler()
            : registers_(0x20000), coils_(0x20000) {}

    modbus::response::read_coils handle(uint8_t, const modbus::request::read_coils& req, modbus::errc_t& modbus_error) {
        modbus::response::read_coils resp;
        resp.values.insert(
                resp.values.end(),
                coils_.cbegin() + req.address,
                coils_.cbegin() + req.address + req.count
        );
        return resp;
    }

    modbus::response::read_discrete_inputs handle(uint8_t, const modbus::request::read_discrete_inputs& req, modbus::errc_t& modbus_error) {
        modbus::response::read_discrete_inputs resp;
        resp.values.resize(req.count);
        return resp;
    }

    modbus::response::read_holding_registers handle(uint8_t, const modbus::request::read_holding_registers& req, modbus::errc_t& modbus_error) {
        modbus::response::read_holding_registers resp;
        resp.values.insert(
                resp.values.end(),
                registers_.cbegin() + req.address,
                registers_.cbegin() + req.address + req.count
        );
        return resp;
    }

    modbus::response::read_input_registers handle(uint8_t, const modbus::request::read_input_registers& req, modbus::errc_t& modbus_error) {
        modbus::response::read_input_registers resp;
        resp.values.resize(req.count);
        return resp;
    }

    modbus::response::write_single_coil handle(uint8_t, const modbus::request::write_single_coil& req, modbus::errc_t& modbus_error) {
        modbus::response::write_single_coil resp;
        coils_[req.address] = req.value;
        resp.address = req.address;
        resp.value = req.value;
        return resp;
    }

    modbus::response::write_single_register handle(uint8_t, const modbus::request::write_single_register& req, modbus::errc_t& modbus_error) {
        modbus::response::write_single_register resp;
        registers_[req.address] = req.value;
        resp.address = req.address;
        resp.value = req.value;
        return resp;
    }

    modbus::response::write_multiple_coils handle(uint8_t, const modbus::request::write_multiple_coils& req, modbus::errc_t& modbus_error) {
        modbus::response::write_multiple_coils resp;
        resp.address = req.address;
        resp.count = 0;
        auto iit = req.values.begin();
        auto oit = coils_.begin() + req.address;
        while (iit < req.values.end() && oit < coils_.end()) {
            *oit++ = *iit++;
            ++resp.count;
        }
        return resp;
    }

    modbus::response::write_multiple_registers handle(uint8_t, const modbus::request::write_multiple_registers& req, modbus::errc_t& modbus_error) {
        modbus::response::write_multiple_registers resp;
        resp.address = req.address;
        resp.count = 0;
        auto iit = req.values.begin();
        auto oit = registers_.begin() + req.address;
        while (iit < req.values.end() && oit < registers_.end()) {
            *oit++ = *iit++;
            ++resp.count;
        }
        return resp;
    }
//private:
    std::vector<std::uint16_t> registers_;
    std::vector<bool> coils_;
};