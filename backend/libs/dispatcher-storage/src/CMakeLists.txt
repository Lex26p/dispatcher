add_library(dispatcher-storage
    src/storage_status.cpp
    src/storage_result.cpp
)

target_include_directories(dispatcher-storage
    PUBLIC
        include
)

target_compile_features(dispatcher-storage PUBLIC cxx_std_20)