add_library(dispatcher-auth-audit
    src/auth_audit_logger.cpp
    src/auth_audit_validator.cpp
    src/auth_authorization_policy.cpp
    src/auth_operation_context.cpp
    src/authorization_audit_recorder.cpp
    src/http_auth_audit_adapter.cpp
    src/in_memory_auth_audit_sink.cpp
    src/runtime_auth_audit_adapter.cpp
)

target_include_directories(dispatcher-auth-audit
    PUBLIC
        include
)

target_compile_features(dispatcher-auth-audit
    PUBLIC
        cxx_std_20
)