//
// Created by omar on 11/16/20.
//
// Copyright (c) 2017, Fizyr (https://fizyr.com)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the copyright holder(s) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>

#include <modbus/client.hpp>
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
