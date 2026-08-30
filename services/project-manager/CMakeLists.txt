find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

find_path(
    PROJECT_MANAGER_BOOST_INCLUDE_DIR
    NAMES boost/beast/websocket.hpp
    REQUIRED
)

pkg_check_modules(
    PROJECT_MANAGER_JSONC
    REQUIRED
    IMPORTED_TARGET
    json-c
)

add_library(dispatcher_project_manager_core STATIC
    src/application.cpp
    src/project_manager.cpp
)

target_include_directories(
    dispatcher_project_manager_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_project_manager_core
    PUBLIC
        Threads::Threads
)

target_compile_features(dispatcher_project_manager_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_project_manager_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_project_manager_sqlite STATIC
    src/sqlite_project_repository.cpp
)

target_include_directories(
    dispatcher_project_manager_sqlite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    dispatcher_project_manager_sqlite
    PUBLIC
        dispatcher_project_manager_core
    PRIVATE
        SQLite::SQLite3
)

target_compile_features(dispatcher_project_manager_sqlite PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_project_manager_sqlite PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(dispatcher_project_manager_service_hub STATIC
    src/authorization_client.cpp
    src/service_hub_provider.cpp
)

target_include_directories(
    dispatcher_project_manager_service_hub
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_MANAGER_BOOST_INCLUDE_DIR}
)

target_link_libraries(
    dispatcher_project_manager_service_hub
    PUBLIC
        dispatcher_project_manager_core
    PRIVATE
        Threads::Threads
        PkgConfig::PROJECT_MANAGER_JSONC
)

target_compile_definitions(
    dispatcher_project_manager_service_hub
    PRIVATE
        BOOST_ERROR_CODE_HEADER_ONLY
)

target_compile_features(dispatcher_project_manager_service_hub PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_project_manager_service_hub PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dispatcher_project_manager
    src/main.cpp
)

target_link_libraries(
    dispatcher_project_manager
    PRIVATE
        dispatcher_project_manager_core
        dispatcher_project_manager_sqlite
        dispatcher_project_manager_service_hub
)

set_target_properties(
    dispatcher_project_manager
    PROPERTIES OUTPUT_NAME "dispatcher-project-manager"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dispatcher_project_manager PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_project_manager_tests
        tests/project_manager_test.cpp
    )
    target_link_libraries(dispatcher_project_manager_tests PRIVATE dispatcher_project_manager_core)
    set_target_properties(dispatcher_project_manager_tests PROPERTIES OUTPUT_NAME "dispatcher-project-manager-tests")

    add_executable(dispatcher_project_manager_persistence_tests
        tests/sqlite_project_repository_test.cpp
    )
    target_link_libraries(dispatcher_project_manager_persistence_tests PRIVATE dispatcher_project_manager_sqlite)
    set_target_properties(dispatcher_project_manager_persistence_tests PROPERTIES OUTPUT_NAME "dispatcher-project-manager-persistence-tests")

    add_executable(dispatcher_project_manager_service_hub_test_client
        tests/service_hub_integration_client.cpp
    )
    target_include_directories(
        dispatcher_project_manager_service_hub_test_client
        PRIVATE
            ${PROJECT_MANAGER_BOOST_INCLUDE_DIR}
    )
    target_link_libraries(
        dispatcher_project_manager_service_hub_test_client
        PRIVATE
            Threads::Threads
            PkgConfig::PROJECT_MANAGER_JSONC
    )
    target_compile_definitions(
        dispatcher_project_manager_service_hub_test_client
        PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
    )
    target_compile_features(dispatcher_project_manager_service_hub_test_client PRIVATE cxx_std_20)
    set_target_properties(
        dispatcher_project_manager_service_hub_test_client
        PROPERTIES OUTPUT_NAME "dispatcher-project-manager-service-hub-test-client"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dispatcher_project_manager_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_project_manager_persistence_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(dispatcher_project_manager_service_hub_test_client PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME project-manager.domain-and-application COMMAND dispatcher_project_manager_tests)
    add_test(NAME project-manager.persistence COMMAND dispatcher_project_manager_persistence_tests)

    add_test(
        NAME project-manager.signal-term
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_project_manager> TERM SIGTERM
    )
    add_test(
        NAME project-manager.signal-int
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_project_manager> INT SIGINT
    )

    if(TARGET dispatcher_service_hub)
        add_dependencies(
            dispatcher_project_manager_service_hub_test_client
            dispatcher_service_hub
            dispatcher_project_manager
        )
        add_test(
            NAME project-manager.service-hub-integration
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/service_hub_integration_test.sh
                $<TARGET_FILE:dispatcher_service_hub>
                $<TARGET_FILE:dispatcher_project_manager>
                $<TARGET_FILE:dispatcher_project_manager_service_hub_test_client>
        )
        set_tests_properties(project-manager.service-hub-integration PROPERTIES TIMEOUT 90)
    endif()

    set_tests_properties(
        project-manager.domain-and-application
        project-manager.persistence
        project-manager.signal-term
        project-manager.signal-int
        PROPERTIES TIMEOUT 15
    )
endif()
