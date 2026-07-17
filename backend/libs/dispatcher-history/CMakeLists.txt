add_library(dispatcher-history
    src/history_sample.cpp
    src/in_memory_history_store.cpp
    src/history_writer.cpp
)

target_include_directories(dispatcher-history
    PUBLIC
        include
)

target_link_libraries(dispatcher-history
    PUBLIC
        dispatcher-core
        dispatcher-domain
        dispatcher-telemetry
)

target_compile_features(dispatcher-history
    PUBLIC
        cxx_std_20
)