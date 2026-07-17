add_library(dispatcher-simulator
    src/telemetry_generator.cpp
)

target_include_directories(dispatcher-simulator
    PUBLIC
        include
)

target_link_libraries(dispatcher-simulator
    PUBLIC
        dispatcher-telemetry
)

target_compile_features(dispatcher-simulator
    PUBLIC
        cxx_std_20
)