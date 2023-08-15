# modbus

A C++23 modbus library using Boost ASIO.


# Features
- Client and server implementations
- coroutine support
- header only

# Using the library

## Client example
```cpp
#include <modbus/client.hpp>

int main(){
    return 0;
}
```

## Server example
```cpp
#include <modbus/server.hpp>

int main(){
    return 0;
}
```

For more examples see [examples](examples/) directory.

# Modbus specification information
- Tcp implementation guide [https://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf](https://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf)
- Conformance test spec [https://www.modbus.org/docs/MBConformanceTestSpec_v3.0.pdf](https://www.modbus.org/docs/MBConformanceTestSpec_v3.0.pdf)
- Spec overview [https://www.modbus.org/specs.php](https://www.modbus.org/specs.php)

# Credits
Based on [https://github.com/fizyr/modbus/](https://github.com/fizyr/modbus/)
Modified to use c++23 coroutines.

See [License](LICENSE) for license information.