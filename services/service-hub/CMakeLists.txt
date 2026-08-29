add_library(dispatcher_service_hub_core STATIC
    src/application.cpp
    src/provider_registry.cpp
)

target_include_directories(dispatcher_service_hub_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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

    add_test(
        NAME service-hub.application
        COMMAND dispatcher_service_hub_tests
    )

    add_test(
        NAME service-hub.provider-registry
        COMMAND dispatcher_service_hub_provider_registry_tests
    )
endif()
