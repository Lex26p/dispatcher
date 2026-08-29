find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)

find_program(
    DATA_HUB_GRPC_CPP_PLUGIN
    NAMES grpc_cpp_plugin
    REQUIRED
)

set(DATA_HUB_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(
    DATA_HUB_PROTO_FILE
    "${DATA_HUB_PROTO_ROOT}/dispatcher/data_hub/v1/data_hub.proto"
)
set(DATA_HUB_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(
    DATA_HUB_GENERATED_BASE
    "${DATA_HUB_GENERATED_DIR}/dispatcher/data_hub/v1/data_hub"
)

set(DATA_HUB_PROTO_SOURCES
    "${DATA_HUB_GENERATED_BASE}.pb.cc"
    "${DATA_HUB_GENERATED_BASE}.grpc.pb.cc"
)

set(DATA_HUB_PROTO_HEADERS
    "${DATA_HUB_GENERATED_BASE}.pb.h"
    "${DATA_HUB_GENERATED_BASE}.grpc.pb.h"
)

add_custom_command(
    OUTPUT
        ${DATA_HUB_PROTO_SOURCES}
        ${DATA_HUB_PROTO_HEADERS}
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${DATA_HUB_GENERATED_DIR}/dispatcher/data_hub/v1"
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE}
        --proto_path=${DATA_HUB_PROTO_ROOT}
        --cpp_out=${DATA_HUB_GENERATED_DIR}
        --grpc_out=${DATA_HUB_GENERATED_DIR}
        --plugin=protoc-gen-grpc=${DATA_HUB_GRPC_CPP_PLUGIN}
        ${DATA_HUB_PROTO_FILE}
    DEPENDS
        ${DATA_HUB_PROTO_FILE}
    COMMENT "Generating Data Hub protobuf and gRPC sources"
    VERBATIM
)

add_library(dispatcher_data_hub_proto STATIC
    ${DATA_HUB_PROTO_SOURCES}
    ${DATA_HUB_PROTO_HEADERS}
)

target_include_directories(dispatcher_data_hub_proto
    PUBLIC
        ${DATA_HUB_GENERATED_DIR}
)

target_link_libraries(dispatcher_data_hub_proto
    PUBLIC
        protobuf::libprotobuf
        PkgConfig::GRPCPP
)

target_compile_features(dispatcher_data_hub_proto PUBLIC cxx_std_20)

add_library(dispatcher_data_hub_core STATIC
    src/application.cpp
    src/current_value_store.cpp
    src/grpc_service.cpp
    src/server.cpp
    src/subscription_manager.cpp
)

target_include_directories(dispatcher_data_hub_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dispatcher_data_hub_core
    PUBLIC
        dispatcher_data_hub_proto
)

target_compile_features(dispatcher_data_hub_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_data_hub_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_data_hub
    src/main.cpp
)

target_link_libraries(dispatcher_data_hub PRIVATE dispatcher_data_hub_core)
set_target_properties(dispatcher_data_hub PROPERTIES OUTPUT_NAME "dispatcher-data-hub")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_data_hub PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_data_hub_tests
        tests/application_test.cpp
    )

    target_link_libraries(dispatcher_data_hub_tests
        PRIVATE
            dispatcher_data_hub_core
    )

    set_target_properties(
        dispatcher_data_hub_tests
        PROPERTIES OUTPUT_NAME "dispatcher-data-hub-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dispatcher_data_hub_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME data-hub.subscription COMMAND dispatcher_data_hub_tests)
endif()
