// Copyright (c) 2023, Skaginn3x (https://skaginn3x.com)

#include <iostream>

#include <modbus/server.hpp>
#include <modbus/default_handler.hpp>

int main(int argc, char* argv[]) {
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

    modbus::server<default_handler> server{ctx, handler, 502};
    server.start();

    std::cout << "Starting server" << std::endl;
    ctx.run();
}

// vim: autoindent syntax=cpp noexpandtab tabstop=4 softtabstop=4 shiftwidth=4
