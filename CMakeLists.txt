cmake_minimum_required(VERSION 3.20)

project(dispatcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DISPATCHER_BUILD_TESTS "Build Dispatcher tests" ON)

if(DISPATCHER_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(services/data-hub)
add_subdirectory(services/service-hub)
add_subdirectory(services/project-manager)
add_subdirectory(services/users-access)

if(
    DISPATCHER_BUILD_TESTS
    AND TARGET dispatcher_project_manager_service_hub_test_client
    AND TARGET dispatcher_users_access
    AND TARGET dispatcher_users_access_project_manager_authorization_fixture
)
    add_dependencies(
        dispatcher_project_manager_service_hub_test_client
        dispatcher_users_access
        dispatcher_users_access_project_manager_authorization_fixture
    )
endif()
