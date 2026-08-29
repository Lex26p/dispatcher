find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

find_path(
    SERVICE_HUB_BOOST_INCLUDE_DIR
    NAMES boost/beast/websocket.hpp
    REQUIRED
)

pkg_check_modules(
    SERVICE_HUB_JSONC
    REQUIRED
    IMPORTED_TARGET
    json-c
)

add_library(dispatcher_service_hub_core STATIC
    src/application.cpp
    src/provider_registry.cpp
    src/server.cpp
)

target_include_directories(dispatcher_service_hub_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${SERVICE_HUB_BOOST_INCLUDE_DIR}
)

target_link_libraries(
    dispatcher_service_hub_core
    PRIVATE
        Threads::Threads
        PkgConfig::SERVICE_HUB_JSONC
)

target_compile_definitions(
    dispatcher_service_hub_core
    PRIVATE
        BOOST_ERROR_CODE_HEADER_ONLY
)

target_compile_features(dispatcher_service_hub_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        dispatcher_service_hub_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_executable(dispatcher_service_hub
    src/main.cpp
)

target_link_libraries(
    dispatcher_service_hub
    PRIVATE
        dispatcher_service_hub_core
)

set_target_properties(
    dispatcher_service_hub
    PROPERTIES OUTPUT_NAME "dispatcher-service-hub"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        dispatcher_service_hub
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_service_hub_tests
        tests/application_test.cpp
    )

    target_link_libraries(
        dispatcher_service_hub_tests
        PRIVATE
            dispatcher_service_hub_core
    )

    set_target_properties(
        dispatcher_service_hub_tests
        PROPERTIES OUTPUT_NAME "dispatcher-service-hub-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_service_hub_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_executable(dispatcher_service_hub_provider_registry_tests
        tests/provider_registry_test.cpp
    )

    target_link_libraries(
        dispatcher_service_hub_provider_registry_tests
        PRIVATE
            dispatcher_service_hub_core
    )

    set_target_properties(
        dispatcher_service_hub_provider_registry_tests
        PROPERTIES OUTPUT_NAME "dispatcher-service-hub-provider-registry-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_service_hub_provider_registry_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_executable(dispatcher_service_hub_request_response_tests
        tests/request_response_test.cpp
    )

    target_include_directories(
        dispatcher_service_hub_request_response_tests
        PRIVATE
            ${SERVICE_HUB_BOOST_INCLUDE_DIR}
    )

    target_link_libraries(
        dispatcher_service_hub_request_response_tests
        PRIVATE
            dispatcher_service_hub_core
            Threads::Threads
            PkgConfig::SERVICE_HUB_JSONC
    )

    target_compile_definitions(
        dispatcher_service_hub_request_response_tests
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )

    set_target_properties(
        dispatcher_service_hub_request_response_tests
        PROPERTIES OUTPUT_NAME "dispatcher-service-hub-request-response-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_service_hub_request_response_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_executable(dispatcher_service_hub_browser_boundary_tests
        tests/browser_boundary_test.cpp
    )

    target_include_directories(
        dispatcher_service_hub_browser_boundary_tests
        PRIVATE
            ${SERVICE_HUB_BOOST_INCLUDE_DIR}
    )

    target_link_libraries(
        dispatcher_service_hub_browser_boundary_tests
        PRIVATE
            dispatcher_service_hub_core
            Threads::Threads
            PkgConfig::SERVICE_HUB_JSONC
    )

    target_compile_definitions(
        dispatcher_service_hub_browser_boundary_tests
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )

    set_target_properties(
        dispatcher_service_hub_browser_boundary_tests
        PROPERTIES OUTPUT_NAME "dispatcher-service-hub-browser-boundary-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_service_hub_browser_boundary_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_executable(dispatcher_service_hub_lifecycle_tests
        tests/lifecycle_and_errors_test.cpp
    )

    target_include_directories(
        dispatcher_service_hub_lifecycle_tests
        PRIVATE
            ${SERVICE_HUB_BOOST_INCLUDE_DIR}
    )

    target_link_libraries(
        dispatcher_service_hub_lifecycle_tests
        PRIVATE
            dispatcher_service_hub_core
            Threads::Threads
            PkgConfig::SERVICE_HUB_JSONC
    )

    target_compile_definitions(
        dispatcher_service_hub_lifecycle_tests
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )

    set_target_properties(
        dispatcher_service_hub_lifecycle_tests
        PROPERTIES OUTPUT_NAME "dispatcher-service-hub-lifecycle-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_service_hub_lifecycle_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_test(
        NAME service-hub.application
        COMMAND dispatcher_service_hub_tests
    )

    add_test(
        NAME service-hub.provider-registry
        COMMAND dispatcher_service_hub_provider_registry_tests
    )

    add_test(
        NAME service-hub.request-response
        COMMAND dispatcher_service_hub_request_response_tests
    )

    add_test(
        NAME service-hub.browser-boundary
        COMMAND dispatcher_service_hub_browser_boundary_tests
    )

    add_test(
        NAME service-hub.lifecycle-and-errors
        COMMAND dispatcher_service_hub_lifecycle_tests
    )

    add_test(
        NAME service-hub.signal-term
        COMMAND
            bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_service_hub>
            TERM
            SIGTERM
    )

    add_test(
        NAME service-hub.signal-int
        COMMAND
            bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_service_hub>
            INT
            SIGINT
    )

    set_tests_properties(
        service-hub.application
        service-hub.provider-registry
        service-hub.request-response
        service-hub.browser-boundary
        service-hub.lifecycle-and-errors
        service-hub.signal-term
        service-hub.signal-int
        PROPERTIES TIMEOUT 15
    )
endif()
