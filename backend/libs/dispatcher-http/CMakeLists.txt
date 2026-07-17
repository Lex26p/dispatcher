add_library(dispatcher-http
    src/http_server_status.cpp
    src/http_server_options.cpp
    src/http_server_result.cpp
    src/http_server_adapter.cpp
    src/http_request_mapper.cpp
    src/http_response_mapper.cpp
    src/http_health_endpoint_handlers.cpp
    src/http_runtime_endpoint_handlers.cpp
    src/http_alarm_endpoint_handlers.cpp
    src/http_endpoint_router.cpp
    src/http_request_dispatcher.cpp
    src/http_server.cpp
    src/http_runtime_contract.cpp
    src/http_alarm_contract.cpp
)

add_library(dispatcher::http ALIAS dispatcher-http)

target_compile_features(dispatcher-http
    PUBLIC
        cxx_std_20
)

target_include_directories(dispatcher-http
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Boost REQUIRED)

target_link_libraries(dispatcher-http
    PUBLIC
        dispatcher-api
        dispatcher-runtime
        Boost::boost
)