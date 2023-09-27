// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#include <iostream>

#include <modbus/server.hpp>
#include <modbus/default_handler.hpp>

int main(int argc, char* argv[]) {
    uint16_t port = 1502;
    if (argc != 1) {
        std::cout << "Using provided port: " << argv[1] << std::endl;
        port = std::atoi(argv[1]);
    }
    asio::io_context ctx;

    auto handler = std::make_shared<default_handler>();
    handler->registers_[0] = 0;
    handler->registers_[1] = 1;
    handler->registers_[2] = 2;
    handler->registers_[3] = 3;
    handler->registers_[4] = 4;
    handler->registers_[5] = 5;
    handler->registers_[6] = 6;
    handler->registers_[7] = 7;
    handler->registers_[8] = 8;
    handler->registers_[9] = 9;
    handler->registers_[10] = 10;
    handler->registers_[11] = 11;
    handler->registers_[12] = 12;
    handler->registers_[13] = 13;
    handler->registers_[14] = 14;

    handler->coils_[0] = false;
    handler->coils_[1] = true;
    handler->coils_[2] = false;
    handler->coils_[3] = true;
    handler->coils_[4] = false;
    handler->coils_[5] = true;
    handler->coils_[6] = false;
    handler->coils_[7] = true;
    handler->coils_[8] = false;
    handler->coils_[9] = true;
    handler->coils_[10] = false;
    handler->coils_[11] = true;
    handler->coils_[12] = false;
    handler->coils_[13] = true;
    handler->coils_[14] = false;

    handler->input_registers_[0] = 0;
    handler->input_registers_[1] = 1;
    handler->input_registers_[2] = 2;
    handler->input_registers_[3] = 3;
    handler->input_registers_[4] = 4;
    handler->input_registers_[5] = 5;
    handler->input_registers_[6] = 6;
    handler->input_registers_[7] = 7;
    handler->input_registers_[8] = 8;
    handler->input_registers_[9] = 9;
    handler->input_registers_[10] = 10;
    handler->input_registers_[11] = 11;
    handler->input_registers_[12] = 12;
    handler->input_registers_[13] = 13;
    handler->input_registers_[14] = 14;

    handler->desc_input[0] = false;
    handler->desc_input[1] = true;
    handler->desc_input[2] = false;
    handler->desc_input[3] = true;
    handler->desc_input[4] = false;
    handler->desc_input[5] = true;
    handler->desc_input[6] = false;
    handler->desc_input[7] = true;
    handler->desc_input[8] = false;
    handler->desc_input[9] = true;
    handler->desc_input[10] = false;
    handler->desc_input[11] = true;
    handler->desc_input[12] = false;
    handler->desc_input[13] = true;
    handler->desc_input[14] = false;

    modbus::server<default_handler> server{ctx, handler, port};
    server.start();

    std::cout << "Starting example server!" << std::endl;
    ctx.run();
}