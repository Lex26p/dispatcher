find_package(Threads REQUIRED)

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
    target_compile_options(
        dispatcher_project_manager_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_executable(dispatcher_project_manager
    src/main.cpp
)

target_link_libraries(
    dispatcher_project_manager
    PRIVATE
        dispatcher_project_manager_core
)

set_target_properties(
    dispatcher_project_manager
    PROPERTIES OUTPUT_NAME "dispatcher-project-manager"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        dispatcher_project_manager
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

if(DISPATCHER_BUILD_TESTS)
    add_executable(dispatcher_project_manager_tests
        tests/project_manager_test.cpp
    )

    target_link_libraries(
        dispatcher_project_manager_tests
        PRIVATE
            dispatcher_project_manager_core
    )

    set_target_properties(
        dispatcher_project_manager_tests
        PROPERTIES OUTPUT_NAME "dispatcher-project-manager-tests"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            dispatcher_project_manager_tests
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_test(
        NAME project-manager.domain-and-application
        COMMAND dispatcher_project_manager_tests
    )

    add_test(
        NAME project-manager.signal-term
        COMMAND
            bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_project_manager>
            TERM
            SIGTERM
    )

    add_test(
        NAME project-manager.signal-int
        COMMAND
            bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/lifecycle_signal_test.sh
            $<TARGET_FILE:dispatcher_project_manager>
            INT
            SIGINT
    )

    set_tests_properties(
        project-manager.domain-and-application
        project-manager.signal-term
        project-manager.signal-int
        PROPERTIES TIMEOUT 15
    )
endif()
