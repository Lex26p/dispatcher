add_library(dispatcher-core
    src/current_state_store.cpp
    src/telemetry_ingestor.cpp
)

target_include_directories(dispatcher-core
    PUBLIC
        include
)

target_link_libraries(dispatcher-core
    PUBLIC
        dispatcher-domain
        dispatcher-telemetry
)

target_compile_features(dispatcher-core
    PUBLIC
        cxx_std_20
)