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

    "uint16_to_bool"_test = [](){
        uint16_to_bool(0xff00);
    };
    return 0;
}
