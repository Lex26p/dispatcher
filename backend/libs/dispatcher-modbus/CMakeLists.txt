add_library(dispatcher-modbus
    src/modbus_frame_codec.cpp
    src/modbus_polling_config.cpp
    src/modbus_register_decoder.cpp
    src/modbus_simulated_tcp_transport.cpp
    src/modbus_tcp_client.cpp
    src/modbus_telemetry_adapter.cpp
)

target_include_directories(dispatcher-modbus
    PUBLIC
        include
)

target_compile_features(dispatcher-modbus
    PUBLIC
        cxx_std_20
)