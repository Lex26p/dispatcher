add_library(dispatcher-storage
    src/storage_status.cpp
    src/storage_result.cpp
    src/history_storage_query.cpp
    src/history_sample_query_result.cpp
    src/alarm_event_storage_query.cpp
    src/alarm_event_query_result.cpp
    src/alarm_acknowledgement_storage_query.cpp
    src/alarm_acknowledgement_query_result.cpp
    src/configuration_storage_query.cpp
    src/configuration_snapshot_query_result.cpp
)

target_include_directories(dispatcher-storage
    PUBLIC
        include
)

target_link_libraries(dispatcher-storage
    PUBLIC
        dispatcher-domain
        dispatcher-history
        dispatcher-alarm
)

target_compile_features(dispatcher-storage PUBLIC cxx_std_20)