# modbus

A C++23 modbus library using Boost ASIO.


# Features
- Client and server implementations
- coroutine support
- header only

# Using the library
see [examples](examples/) directory.

# Future improvements
- Serial support
- verify functionality on big endian systems
- conformance test
- Multiple outstanding transactions for clients.
    Currently, the client only supports a single outstanding connection at a time.

# Modbus specification links
- Application protocol [https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf](https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
- Tcp implementation guide [https://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf](https://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf)
- Conformance test spec [https://www.modbus.org/docs/MBConformanceTestSpec_v3.0.pdf](https://www.modbus.org/docs/MBConformanceTestSpec_v3.0.pdf)
- Spec overview [https://www.modbus.org/specs.php](https://www.modbus.org/specs.php)

# Credits
Based on [https://github.com/fizyr/modbus/](https://github.com/fizyr/modbus/)
Modified to use c++23 coroutines.

See [License](LICENSE) for license information.
