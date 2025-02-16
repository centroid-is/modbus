// Copyright (c) 2025, Centroid ehf. (https://centroid.is)

#include <modbus/server.hpp>
#include <modbus/default_handler.hpp>

int main() {
  asio::io_context server_ctx;

  auto handler = std::make_shared<modbus::default_handler>();
  modbus::server server(server_ctx, handler, 1502);
  server.start();
  server_ctx.run();
}
