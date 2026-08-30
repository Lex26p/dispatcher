find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

find_path(
    USERS_ACCESS_BOOST_INCLUDE_DIR
    NAMES boost/beast/websocket.hpp
    REQUIRED
)

pkg_check_modules(
    USERS_ACCESS_JSONC
    REQUIRED
    IMPORTED_TARGET
    json-c
)

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

add_library(dispatcher_users_access_administration STATIC
    src/administration.cpp
)

target_include_directories(
    dispatcher_users_access_administration
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_administration
    PUBLIC
        dispatcher_users_access_core
)

target_compile_features(dispatcher_users_access_administration PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_administration PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_users_access_sqlite_administration STATIC
    src/sqlite_administration_store.cpp
)

target_include_directories(
    dispatcher_users_access_sqlite_administration
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_users_access_sqlite_administration
    PUBLIC
        dispatcher_users_access_administration
    PRIVATE
        SQLite::SQLite3
)

target_compile_features(dispatcher_users_access_sqlite_administration PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_sqlite_administration PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_users_access_service_hub STATIC
    src/service_hub_provider.cpp
)

target_include_directories(
    dispatcher_users_access_service_hub
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${USERS_ACCESS_BOOST_INCLUDE_DIR}
)

target_link_libraries(
    dispatcher_users_access_service_hub
    PUBLIC
        dispatcher_users_access_administration
        dispatcher_users_access_session
    PRIVATE
        Threads::Threads
        PkgConfig::USERS_ACCESS_JSONC
)

target_compile_definitions(
    dispatcher_users_access_service_hub
    PRIVATE
        BOOST_ERROR_CODE_HEADER_ONLY
)

target_compile_features(dispatcher_users_access_service_hub PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_users_access_service_hub PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_users_access
    src/main.cpp
)

target_link_libraries(
    dispatcher_users_access
    PRIVATE
        dispatcher_users_access_administration
        dispatcher_users_access_bootstrap
        dispatcher_users_access_core
        dispatcher_users_access_crypto
        dispatcher_users_access_session
        dispatcher_users_access_service_hub
        dispatcher_users_access_sqlite
        dispatcher_users_access_sqlite_administration
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

    add_executable(dispatcher_users_access_administration_tests
        tests/administration_test.cpp
    )
    target_link_libraries(
        dispatcher_users_access_administration_tests
        PRIVATE
            dispatcher_users_access_administration
            dispatcher_users_access_bootstrap
            dispatcher_users_access_core
            dispatcher_users_access_crypto
            dispatcher_users_access_sqlite
            dispatcher_users_access_sqlite_administration
            SQLite::SQLite3
    )
    set_target_properties(
        dispatcher_users_access_administration_tests
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-administration-tests"
    )

    add_executable(dispatcher_users_access_project_manager_authorization_fixture
        tests/project_manager_authorization_fixture.cpp
    )
    target_link_libraries(
        dispatcher_users_access_project_manager_authorization_fixture
        PRIVATE
            dispatcher_users_access_core
            dispatcher_users_access_crypto
            dispatcher_users_access_sqlite
            SQLite::SQLite3
    )
    target_compile_features(
        dispatcher_users_access_project_manager_authorization_fixture
        PRIVATE cxx_std_20
    )
    set_target_properties(
        dispatcher_users_access_project_manager_authorization_fixture
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-project-manager-authorization-fixture"
    )

    add_executable(dispatcher_users_access_service_hub_test_client
        tests/service_hub_integration_client.cpp
    )
    target_include_directories(
        dispatcher_users_access_service_hub_test_client
        PRIVATE
            ${USERS_ACCESS_BOOST_INCLUDE_DIR}
    )
    target_link_libraries(
        dispatcher_users_access_service_hub_test_client
        PRIVATE
            Threads::Threads
            PkgConfig::USERS_ACCESS_JSONC
    )
    target_compile_definitions(
        dispatcher_users_access_service_hub_test_client
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )
    target_compile_features(dispatcher_users_access_service_hub_test_client PRIVATE cxx_std_20)
    set_target_properties(
        dispatcher_users_access_service_hub_test_client
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-service-hub-test-client"
    )

    add_executable(dispatcher_users_access_administration_integration_client
        tests/administration_integration_client.cpp
    )
    target_include_directories(
        dispatcher_users_access_administration_integration_client
        PRIVATE
            ${USERS_ACCESS_BOOST_INCLUDE_DIR}
    )
    target_link_libraries(
        dispatcher_users_access_administration_integration_client
        PRIVATE
            Threads::Threads
            PkgConfig::USERS_ACCESS_JSONC
    )
    target_compile_definitions(
        dispatcher_users_access_administration_integration_client
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )
    target_compile_features(
        dispatcher_users_access_administration_integration_client
        PRIVATE cxx_std_20
    )
    set_target_properties(
        dispatcher_users_access_administration_integration_client
        PROPERTIES OUTPUT_NAME "dispatcher-users-access-administration-integration-client"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dispatcher_users_access_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_persistence_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_session_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_administration_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_project_manager_authorization_fixture PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_service_hub_test_client PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_users_access_administration_integration_client PRIVATE -Wall -Wextra -Wpedantic)
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
        NAME users-access.administration
        COMMAND dispatcher_users_access_administration_tests
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

    if(TARGET dispatcher_service_hub)
        add_dependencies(
            dispatcher_users_access_service_hub_test_client
            dispatcher_service_hub
            dispatcher_users_access
        )
        add_dependencies(
            dispatcher_users_access_administration_integration_client
            dispatcher_service_hub
            dispatcher_users_access
        )
        add_test(
            NAME users-access.service-hub-integration
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/service_hub_integration_test.sh
                $<TARGET_FILE:dispatcher_service_hub>
                $<TARGET_FILE:dispatcher_users_access>
                $<TARGET_FILE:dispatcher_users_access_service_hub_test_client>
        )
        set_tests_properties(users-access.service-hub-integration PROPERTIES TIMEOUT 45)

        add_test(
            NAME users-access.administration-integration
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/administration_integration_test.sh
                $<TARGET_FILE:dispatcher_service_hub>
                $<TARGET_FILE:dispatcher_users_access>
                $<TARGET_FILE:dispatcher_users_access_administration_integration_client>
        )
        set_tests_properties(users-access.administration-integration PROPERTIES TIMEOUT 60)
    endif()

    set_tests_properties(
        users-access.domain-and-application
        users-access.persistence-and-credentials
        users-access.bootstrap-cli
        users-access.authentication-and-session
        users-access.administration
        users-access.signal-term
        users-access.signal-int
        PROPERTIES TIMEOUT 30
    )
endif()
