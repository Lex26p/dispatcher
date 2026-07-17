find_package(spdlog CONFIG REQUIRED)

add_executable(dispatcher-smoke
    main.cpp
)

target_link_libraries(dispatcher-smoke
    PRIVATE
        dispatcher-common
        spdlog::spdlog
)

target_compile_features(dispatcher-smoke
    PRIVATE
        cxx_std_20
)