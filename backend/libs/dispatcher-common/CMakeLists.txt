add_library(dispatcher-common
    src/version.cpp
)

target_include_directories(dispatcher-common
    PUBLIC
        include
)

target_compile_features(dispatcher-common
    PUBLIC
        cxx_std_20
)