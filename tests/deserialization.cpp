#include <array>

#include <boost/ut.hpp>
#include <modbus/impl/deserialize_base.hpp>


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
    return 0;
}
