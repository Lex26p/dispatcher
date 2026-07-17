find_package(spdlog CONFIG REQUIRED)

add_executable(dispatcher-e2e-smoke
    main.cpp
)

target_link_libraries(dispatcher-e2e-smoke
    PRIVATE
        dispatcher-runtime
        dispatcher-simulator
        spdlog::spdlog
)

target_compile_features(dispatcher-e2e-smoke
    PRIVATE
        cxx_std_20
)