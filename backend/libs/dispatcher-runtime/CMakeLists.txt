add_library(dispatcher-runtime
    src/dispatcher_runtime.cpp
    src/health_status.cpp
    src/readiness_status.cpp
    src/health_check_result.cpp
    src/health_snapshot.cpp
    src/diagnostic_severity.cpp
    src/diagnostic_record.cpp
    src/runtime_diagnostics_snapshot.cpp
    src/runtime_consistency_status.cpp
    src/runtime_consistency_issue.cpp
    src/runtime_consistency_snapshot.cpp
    src/runtime_hardening_status.cpp
    src/runtime_hardening_snapshot.cpp
)

target_include_directories(dispatcher-runtime
    PUBLIC
        include
)

target_link_libraries(dispatcher-runtime
    PUBLIC
        dispatcher-core
        dispatcher-history
        dispatcher-alarm
        dispatcher-storage
)

target_compile_features(dispatcher-runtime PUBLIC cxx_std_20)