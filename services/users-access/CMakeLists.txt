find_package(Threads REQUIRED)

add_library(dispatcher_users_access_core STATIC
    src/application.cpp
    src/users_access_manager.cpp
)

target_include_directories(
    dispatcher_users_access_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_core
    PUBLIC
        Threads::Threads
)

target_compile_features(dispatcher_users_access_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_users_access
    src/main.cpp
)

target_link_libraries(
    dispatcher_users_access
    PRIVATE
        dispatcher_users_access_core
)

set_target_properties(
    dispatcher_users_access
    PROPERTIES OUTPUT_NAME "dispatcher-users-access"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_users_access_tests
        tests/users_access_test.cpp
    )
    target_link_libraries(dispatcher_users_access_tests PRIVATE dispatcher_users_access_core)
    set_target_properties(
        dispatcher_users_access_tests
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dispatcher_users_access_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME users-access.domain-and-application
        COMMAND dispatcher_users_access_tests
    )
    add_test(
        NAME users-access.signal-term
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_users_access> TERM SIGTERM
    )
    add_test(
        NAME users-access.signal-int
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_users_access> INT SIGINT
    )

    set_tests_properties(
        users-access.domain-and-application
        users-access.signal-term
        users-access.signal-int
        PROPERTIES TIMEOUT 15
    )
endif()
