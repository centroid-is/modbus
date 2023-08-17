#include <array>

#include <boost/ut.hpp>
#include <modbus/tcp.hpp>
#include <modbus/impl/deserialize_base.hpp>
#include <modbus/impl/deserialize_request.hpp>


void print_bytes(std::span<uint8_t> data){
    for (auto& k : data) {
        std::cout << static_cast<int>(k) << " ";
    }
    std::cout << std::endl;
}

using namespace modbus::impl;
int main(){
    using boost::ut::operator""_test;
    using boost::ut::expect;

    "check_length"_test = []() {
        auto error = check_length(0, 0);
        expect(!error);
        error = check_length(0, 1);
        expect(error);
        error = check_length(1, 0);
        expect(!error);
    };

    "deserialize_be16"_test = [](){
        auto data = std::array<uint8_t, 2>{0x00, 0x01};
        expect(deserialize_be16(std::span<uint8_t>(data)) == 1);

        data = std::array<uint8_t, 2>{0x00, 0x02};
        expect(deserialize_be16(std::span<uint8_t>(data)) == 2);
    };

    "deserialize_bit_list"_test = [](){
        auto data = std::array<uint8_t, 2> {0xF0, 0x0F};
        auto bits = deserialize_bit_list(data, 16).value();

        expect(bits.size() == 16) << bits.size();
        expect(bits[0] == false);
        expect(bits[1] == false);
        expect(bits[2] == false);
        expect(bits[3] == false);
        expect(bits[4] == true);
        expect(bits[5] == true);
        expect(bits[6] == true);
        expect(bits[7] == true);
        expect(bits[8] == true);
        expect(bits[9] == true);
        expect(bits[10] == true);
        expect(bits[11] == true);
        expect(bits[12] == false);
        expect(bits[13] == false);
        expect(bits[14] == false);
        expect(bits[15] == false);
    };

    "deserialize_word_list"_test = [](){
        auto data = uint16_t{0x0001};
        auto words = deserialize_word_list(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&data), 2), 1).value();
        expect(words.size() == 1);
        expect(words[0] == 256) << words[0];

        data = uint16_t{0x0100};
        words = deserialize_word_list(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&data), 2), 1).value();
        expect(words.size() == 1);
        expect(words[0] == 1) << words[0];

        std::array<uint16_t, 2> data2 = {0x0100, 0x0001};
        words = deserialize_word_list(std::span<uint8_t>(reinterpret_cast<uint8_t*>(data2.data()), 4), 2).value();
        expect(words.size() == 2);
        expect(words[0] == 1) << words[0];
        expect(words[1] == 256) << words[1];
    };

    // ATH, mbpoll -r parameter is from 1 but the modbus addresses
    // are from 0. So the address 1 in mbpoll is 0 in modbus.

    "deserialize request read_coils"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -c 15 -a 56 -t 0 -r 15
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x1, 0x0, 0xe, 0x0, 0xf};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::read_coils);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::read_coils>(expected_request.value()));
        auto request = std::get<modbus::request::read_coils>(expected_request.value());
        expect(request.function == modbus::function_t::read_coils);
        expect(request.address == 14) << request.address;
        expect(request.count ==  15) << request.count;
    };

    "deserialize request read_discrete_inputs"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -c 15 -a 56 -t 1
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x2, 0x0, 0x0, 0x0, 0xf};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::read_discrete_inputs);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::read_discrete_inputs>(expected_request.value()));
        auto request = std::get<modbus::request::read_discrete_inputs>(expected_request.value());
        expect(request.function == modbus::function_t::read_discrete_inputs);
        expect(request.address == 0x0000);
        expect(request.count ==  15);
    };

    "deserialize header & request read_holding_registers"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -c 15 -a 56
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x3, 0x0, 0x0, 0x0, 0xf};

        auto header = modbus::tcp_mbap::from_bytes(data);
        expect(header.transaction == 1);
        expect(header.protocol == 0);
        expect(header.length == data.size() - modbus::tcp_mbap::size + 1);
        expect(header.unit == 56);

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::read_holding_registers);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::read_holding_registers>(expected_request.value()));
        auto request = std::get<modbus::request::read_holding_registers>(expected_request.value());
        expect(request.function == modbus::function_t::read_holding_registers);
        expect(request.address == 0x0000);
        expect(request.count ==  15);
    };
    "deserialize request read_input_registers"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -c 15 -a 56 -t 3
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x4, 0x0, 0x0, 0x0, 0xf};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::read_input_registers);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::read_input_registers>(expected_request.value()));
        auto request = std::get<modbus::request::read_input_registers>(expected_request.value());
        expect(request.function == modbus::function_t::read_input_registers);
        expect(request.address == 0x0000);
        expect(request.count ==  15);
    };
    "deserialize request write_single_coil"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -a 56 -t 0 -r 15 0
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x5, 0x0, 0xe, 0x0, 0x0};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::write_single_coil);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::write_single_coil>(expected_request.value()));
        auto request = std::get<modbus::request::write_single_coil>(expected_request.value());
        expect(request.function == modbus::function_t::write_single_coil);
        expect(request.address == 14);
        expect(request.value == false);
    };

    "deserialize request write_single_register"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -a 56 -t 4 -r 15 1556
        auto data = std::array<uint8_t, 12> {0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x38, 0x6, 0x0, 0xe, 0x6, 0x14};


        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::write_single_register);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::write_single_register>(expected_request.value()));
        auto request = std::get<modbus::request::write_single_register>(expected_request.value());
        expect(request.function == modbus::function_t::write_single_register);
        expect(request.address == 14);
        expect(request.value == 1556);
    };

    "deserialize request write_multiple_coils"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -a 56 -t 0 -r 15 1 0 1 0 1 0 1 0 1 0
        auto data = std::array<uint8_t, 15> {0x0, 0x1, 0x0, 0x0, 0x0, 0x9, 0x38, 0xf, 0x0, 0xe, 0x0, 0xa, 0x2, 0x55, 0x1};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::write_multiple_coils);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::write_multiple_coils>(expected_request.value()));
        auto request = std::get<modbus::request::write_multiple_coils>(expected_request.value());
        expect(request.function == modbus::function_t::write_multiple_coils);
        expect(request.address == 14);
        expect(request.values.size() == 10);
        expect(request.values[0]);
        expect(!request.values[1]);
        expect(request.values[2]);
        expect(!request.values[3]);
        expect(request.values[4]);
        expect(!request.values[5]);
        expect(request.values[6]);
        expect(!request.values[7]);
        expect(request.values[8]);
        expect(!request.values[9]);
    };
    "deserialize request write_multiple_registers"_test = [](){
        // Request captured from mbpoll cli program.
        // mbpoll localhost -p 502 -m tcp -l 4000 -a 56 -t 4 -r 15 1556
        auto data = std::array<uint8_t, 21> {0x0, 0x1, 0x0, 0x0, 0x0, 0xf, 0x38, 0x10, 0x0, 0xe, 0x0, 0x4, 0x8, 0x6, 0x14, 0x6, 0x15, 0x6, 0x16, 0x6, 0x17};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::write_multiple_registers);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::write_multiple_registers>(expected_request.value()));
        auto request = std::get<modbus::request::write_multiple_registers>(expected_request.value());
        expect(request.function == modbus::function_t::write_multiple_registers);
        expect(request.address == 14);
        expect(request.values.size() == 4);
        expect(request.values[0] == 1556);
        expect(request.values[1] == 1557);
        expect(request.values[2] == 1558);
        expect(request.values[3] == 1559);
    };

    "deserialize request mask_write_register"_test = [](){
        // Request captured from 'helpers/mask_write_register.c'
        auto data = std::array<uint8_t, 14> {0x0, 0x1, 0x0, 0x0, 0x0, 0x8, 0xff, 0x16, 0x0, 0xe, 0x0, 0xf, 0x0, 0x10};

        auto expected_request = deserialize_request(std::span(data).subspan(modbus::tcp_mbap::size), modbus::function_t::mask_write_register);
        expect(expected_request.has_value());
        expect(holds_alternative<modbus::request::mask_write_register>(expected_request.value()));
        auto request = std::get<modbus::request::mask_write_register>(expected_request.value());
        expect(request.function == modbus::function_t::mask_write_register);
        expect(request.address == 14);
        expect(request.and_mask == 15);
        expect(request.or_mask == 16);
    };
    return 0;
}
