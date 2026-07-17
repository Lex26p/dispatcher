add_executable(dispatcher-http-server
    src/main.cpp
)

target_link_libraries(dispatcher-http-server
    PRIVATE
        dispatcher-http
        dispatcher-runtime
        dispatcher-api
)

target_compile_features(dispatcher-http-server
    PRIVATE
        cxx_std_20
)