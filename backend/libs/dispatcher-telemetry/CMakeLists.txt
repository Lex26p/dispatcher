add_library(dispatcher-telemetry
    src/telemetry_value.cpp
    src/telemetry_adapter_status.cpp
    src/telemetry_adapter_error.cpp
    src/telemetry_adapter_result.cpp
    src/telemetry_adapter_capability.cpp
    src/telemetry_adapter.cpp
    src/telemetry_read_request.cpp
    src/telemetry_read_result.cpp
    src/telemetry_write_request.cpp
    src/telemetry_write_result.cpp
    src/telemetry_adapter_registry.cpp
    src/telemetry_adapter_operations.cpp
)

target_include_directories(dispatcher-telemetry
    PUBLIC
        include
)

target_link_libraries(dispatcher-telemetry
    PUBLIC
        dispatcher-domain
)

target_compile_features(dispatcher-telemetry
    PUBLIC
        cxx_std_20
)