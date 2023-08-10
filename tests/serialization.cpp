#include <array>

#include <boost/ut.hpp>
#include <modbus/tcp.hpp>

void print_bytes(std::span<uint8_t> data){
    for (auto& k : data) {
        std::cout << static_cast<int>(k) << " ";
    }
    std::cout << std::endl;
}

int main(){
    using boost::ut::operator""_test;
    using boost::ut::expect;

    "Modbus tcp header serialize/deserialize"_test = []() {
        using modbus::tcp_mbap;
        tcp_mbap header;
        auto bytes = std::array<uint8_t, tcp_mbap::size>{0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01};
        auto hd = tcp_mbap::from_bytes(bytes);

        expect(hd.transaction == 1);
        expect(hd.protocol == 0);
        expect(hd.length == 6);
        expect(hd.unit == 1);

        auto buffer = hd.to_bytes();
        print_bytes(buffer);
        print_bytes(bytes);
        for (size_t i = 0; i < buffer.size(); i++)
            expect(buffer[i] == bytes[i]);
    };
    return 0;
}