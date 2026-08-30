find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(SQLite3 REQUIRED)
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

add_library(dispatcher_users_access_crypto STATIC
    src/openssl_scrypt_password_hasher.cpp
    src/openssl_session_token_codec.cpp
)

target_include_directories(
    dispatcher_users_access_crypto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_crypto
    PRIVATE
        OpenSSL::Crypto
)

target_compile_features(dispatcher_users_access_crypto PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_crypto PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_users_access_bootstrap STATIC
    src/bootstrap.cpp
)

target_include_directories(
    dispatcher_users_access_bootstrap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_bootstrap
    PUBLIC
        dispatcher_users_access_core
        dispatcher_users_access_crypto
)

target_compile_features(dispatcher_users_access_bootstrap PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_bootstrap PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_users_access_sqlite STATIC
    src/sqlite_users_access_repository.cpp
)

target_include_directories(
    dispatcher_users_access_sqlite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_sqlite
    PUBLIC
        dispatcher_users_access_core
    PRIVATE
        SQLite::SQLite3
)

target_compile_features(dispatcher_users_access_sqlite PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_sqlite PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_users_access_session STATIC
    src/authentication_session_service.cpp
)

target_include_directories(
    dispatcher_users_access_session
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_session
    PUBLIC
        dispatcher_users_access_core
)

target_compile_features(dispatcher_users_access_session PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_session PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_users_access
    src/main.cpp
)

target_link_libraries(
    dispatcher_users_access
    PRIVATE
        dispatcher_users_access_bootstrap
        dispatcher_users_access_core
        dispatcher_users_access_crypto
        dispatcher_users_access_sqlite
        OpenSSL::Crypto
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

    add_executable(dispatcher_users_access_persistence_tests
        tests/persistence_test.cpp
    )
    target_link_libraries(
        dispatcher_users_access_persistence_tests
        PRIVATE
            dispatcher_users_access_bootstrap
            dispatcher_users_access_core
            dispatcher_users_access_crypto
            dispatcher_users_access_sqlite
    )
    set_target_properties(
        dispatcher_users_access_persistence_tests
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-persistence-tests"
    )

    add_executable(dispatcher_users_access_session_tests
        tests/session_test.cpp
    )
    target_link_libraries(
        dispatcher_users_access_session_tests
        PRIVATE
            dispatcher_users_access_bootstrap
            dispatcher_users_access_core
            dispatcher_users_access_crypto
            dispatcher_users_access_session
            dispatcher_users_access_sqlite
    )
    set_target_properties(
        dispatcher_users_access_session_tests
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-session-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dispatcher_users_access_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_persistence_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_session_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME users-access.domain-and-application
        COMMAND dispatcher_users_access_tests
    )
    add_test(
        NAME users-access.persistence-and-credentials
        COMMAND dispatcher_users_access_persistence_tests
    )
    add_test(
        NAME users-access.bootstrap-cli
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/bootstrap_cli_test.sh
            $<TARGET_FILE:dispatcher_users_access>
    )
    add_test(
        NAME users-access.authentication-and-session
        COMMAND dispatcher_users_access_session_tests
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
        users-access.persistence-and-credentials
        users-access.bootstrap-cli
        users-access.authentication-and-session
        users-access.signal-term
        users-access.signal-int
        PROPERTIES TIMEOUT 30
    )
endif()
