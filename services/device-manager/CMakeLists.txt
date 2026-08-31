find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_library(dispatcher_device_manager_domain STATIC
    src/domain.cpp
)

target_include_directories(
    dispatcher_device_manager_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(dispatcher_device_manager_domain PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_device_manager_domain PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_device_manager_storage STATIC
    src/sqlite_metadata_repository.cpp
)

target_include_directories(
    dispatcher_device_manager_storage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_device_manager_storage
    PUBLIC
        dispatcher_device_manager_domain
    PRIVATE
        SQLite::SQLite3
)

target_compile_features(dispatcher_device_manager_storage PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_device_manager_storage PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_device_manager_application STATIC
    src/application.cpp
)

target_include_directories(
    dispatcher_device_manager_application
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_device_manager_application
    PUBLIC
        Threads::Threads
)

target_compile_features(dispatcher_device_manager_application PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_device_manager_application PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_device_manager
    src/main.cpp
)

target_link_libraries(
    dispatcher_device_manager
    PRIVATE
        dispatcher_device_manager_application
        dispatcher_device_manager_domain
        dispatcher_device_manager_storage
)

set_target_properties(
    dispatcher_device_manager
    PROPERTIES OUTPUT_NAME "dispatcher-device-manager"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_device_manager PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_device_manager_domain_tests
        tests/domain_test.cpp
    )
    target_link_libraries(
        dispatcher_device_manager_domain_tests
        PRIVATE dispatcher_device_manager_domain
    )
    set_target_properties(
        dispatcher_device_manager_domain_tests
        PROPERTIES OUTPUT_NAME "dispatcher-device-manager-domain-tests"
    )

    add_executable(dispatcher_device_manager_persistence_tests
        tests/sqlite_metadata_repository_test.cpp
    )
    target_link_libraries(
        dispatcher_device_manager_persistence_tests
        PRIVATE dispatcher_device_manager_storage
    )
    set_target_properties(
        dispatcher_device_manager_persistence_tests
        PROPERTIES OUTPUT_NAME "dispatcher-device-manager-persistence-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_device_manager_domain_tests
            PRIVATE -Wall -Wextra -Wpedantic
        )
        target_compile_options(
            dispatcher_device_manager_persistence_tests
            PRIVATE -Wall -Wextra -Wpedantic
        )
    endif()

    add_test(
        NAME device-manager.domain
        COMMAND dispatcher_device_manager_domain_tests
    )
    add_test(
        NAME device-manager.persistence
        COMMAND dispatcher_device_manager_persistence_tests
    )
    add_test(
        NAME device-manager.signal-term
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_device_manager> TERM SIGTERM
    )
    add_test(
        NAME device-manager.signal-int
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_device_manager> INT SIGINT
    )

    set_tests_properties(
        device-manager.domain
        device-manager.persistence
        device-manager.signal-term
        device-manager.signal-int
        PROPERTIES TIMEOUT 15
    )
endif()
