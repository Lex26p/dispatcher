find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(dispatcher-storage-sqlite
    src/sqlite_alarm_event_storage.cpp
    src/sqlite_configuration_snapshot_storage.cpp
    src/sqlite_database.cpp
    src/sqlite_history_storage.cpp
    src/sqlite_migration_runner.cpp
    src/sqlite_storage_context.cpp
)

target_include_directories(dispatcher-storage-sqlite
    PUBLIC
        include
)

target_link_libraries(dispatcher-storage-sqlite
    PUBLIC
        dispatcher-storage
    PRIVATE
        unofficial::sqlite3::sqlite3
)

target_compile_features(dispatcher-storage-sqlite
    PUBLIC
        cxx_std_20
)