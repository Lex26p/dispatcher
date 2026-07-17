include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

FetchContent_MakeAvailable(googletest)

add_executable(dispatcher-common-tests
    common_version_tests.cpp
)

target_link_libraries(dispatcher-common-tests
    PRIVATE
        dispatcher-common
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-common-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-common-tests
    COMMAND dispatcher-common-tests
)

add_executable(dispatcher-storage-tests
    storage_alarm_acknowledgement_tests.cpp
    storage_alarm_event_tests.cpp
    storage_configuration_tests.cpp
    storage_history_tests.cpp
    storage_in_memory_repository_tests.cpp
    storage_repository_tests.cpp
    storage_result_tests.cpp
)

target_link_libraries(dispatcher-storage-tests
    PRIVATE
        dispatcher-storage
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-storage-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-storage-tests
    COMMAND dispatcher-storage-tests
)

add_executable(dispatcher-domain-tests
    domain_tag_definition_tests.cpp
    domain_tag_validation_tests.cpp
    domain_tag_definition_builder_tests.cpp
    domain_tag_address_tests.cpp
    domain_tag_catalog_tests.cpp
    domain_tag_naming_tests.cpp
    domain_device_definition_tests.cpp
    domain_device_catalog_tests.cpp
    domain_configuration_snapshot_tests.cpp
    domain_configuration_snapshot_builder_tests.cpp
    domain_configuration_metadata_tests.cpp
    configuration_snapshot_read_access_tests.cpp
    operator_identity_tests.cpp
    operator_authorization_tests.cpp
    operator_directory_tests.cpp
    operator_session_tests.cpp
    operator_auth_context_tests.cpp
    operator_auth_header_tests.cpp
    notification_model_tests.cpp
    notification_route_tests.cpp
    notification_route_table_tests.cpp
    notification_delivery_tests.cpp
    notification_dispatcher_tests.cpp
    notification_routing_header_tests.cpp
)

add_executable(dispatcher-api-tests
    api_alarm_tests.cpp
    api_configuration_tests.cpp
    api_dispatcher_facade_tests.cpp
    api_history_tests.cpp
    api_page_tests.cpp
    api_result_tests.cpp
    api_runtime_tests.cpp
    api_status_mapping_tests.cpp
    api_telemetry_tests.cpp
    api_configuration_io_tests.cpp
    transport_dto_tests.cpp
    transport_endpoint_tests.cpp
    transport_router_tests.cpp
    transport_endpoint_catalog_tests.cpp
    transport_adapter_tests.cpp
    transport_header_tests.cpp
)

target_link_libraries(dispatcher-api-tests
    PRIVATE
        dispatcher-api
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-api-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-api-tests
    COMMAND dispatcher-api-tests
)

add_executable(dispatcher-config-tests
    config_document_tests.cpp
    config_export_mapper_tests.cpp
    config_export_model_tests.cpp
    config_exporter_tests.cpp
    config_import_model_mapper_tests.cpp
    config_import_model_tests.cpp
    config_importer_tests.cpp
    config_json_export_serializer_tests.cpp
    config_json_import_parser_tests.cpp
    config_result_tests.cpp
    config_import_export_roundtrip_tests.cpp
)

target_link_libraries(dispatcher-config-tests
    PRIVATE
        dispatcher-config
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-config-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-config-tests
    COMMAND dispatcher-config-tests
)

target_link_libraries(dispatcher-domain-tests
    PRIVATE
        dispatcher-domain
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-domain-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-domain-tests
    COMMAND dispatcher-domain-tests
)

add_executable(dispatcher-telemetry-tests
    telemetry_value_tests.cpp
    telemetry_adapter_result_tests.cpp
    telemetry_adapter_interface_tests.cpp
    telemetry_adapter_read_model_tests.cpp
    telemetry_adapter_write_model_tests.cpp
    telemetry_adapter_registry_tests.cpp
    telemetry_adapter_operations_tests.cpp
    telemetry_adapter_interfaces_header_tests.cpp
)

target_link_libraries(dispatcher-telemetry-tests
    PRIVATE
        dispatcher-telemetry
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-telemetry-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-telemetry-tests
    COMMAND dispatcher-telemetry-tests
)

add_executable(dispatcher-core-tests
    current_state_store_tests.cpp
    telemetry_ingestor_tests.cpp
)

target_link_libraries(dispatcher-core-tests
    PRIVATE
        dispatcher-core
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-core-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-core-tests
    COMMAND dispatcher-core-tests
)

add_executable(dispatcher-simulator-tests
    telemetry_generator_tests.cpp
)

target_link_libraries(dispatcher-simulator-tests
    PRIVATE
        dispatcher-simulator
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-simulator-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-simulator-tests
    COMMAND dispatcher-simulator-tests
)

add_executable(dispatcher-history-tests
    history_in_memory_store_tests.cpp
    history_writer_tests.cpp
)

target_link_libraries(dispatcher-history-tests
    PRIVATE
        dispatcher-history
        GTest::gtest_main
)

add_test(
    NAME dispatcher-history-tests
    COMMAND dispatcher-history-tests
)

add_executable(dispatcher-alarm-tests
    alarm_acknowledgement_command_tests.cpp
    alarm_acknowledgement_command_validation_tests.cpp
    alarm_acknowledgement_model_tests.cpp
    alarm_acknowledgement_store_tests.cpp
    alarm_batch_evaluation_tests.cpp
    alarm_catalog_tests.cpp
    alarm_condition_catalog_tests.cpp
    alarm_configuration_snapshot_tests.cpp
    alarm_configured_evaluation_tests.cpp
    alarm_definition_tests.cpp
    alarm_evaluator_tests.cpp
    alarm_event_store_tests.cpp
    alarm_operator_snapshot_tests.cpp
    alarm_runtime_acknowledge_tests.cpp
    alarm_runtime_acknowledgement_audit_tests.cpp
    alarm_runtime_index_tests.cpp
    alarm_runtime_reload_prune_tests.cpp
    alarm_runtime_reload_tests.cpp
    alarm_runtime_snapshot_tests.cpp
    alarm_runtime_state_query_tests.cpp
    alarm_runtime_tests.cpp
    alarm_state_store_tests.cpp
    alarm_state_transition_tests.cpp
    alarm_threshold_condition_tests.cpp
    alarm_suppression_model_tests.cpp
    alarm_suppression_store_tests.cpp
    alarm_suppression_snapshot_tests.cpp
    alarm_runtime_suppression_tests.cpp
    alarm_runtime_suppression_evaluation_tests.cpp
    alarm_operator_suppression_snapshot_tests.cpp
)

target_link_libraries(dispatcher-alarm-tests
    PRIVATE
        dispatcher-alarm
        GTest::gtest_main
)

add_test(
    NAME dispatcher-alarm-tests
    COMMAND dispatcher-alarm-tests
)

add_executable(dispatcher-runtime-tests
    dispatcher_runtime_acknowledgement_tests.cpp
    dispatcher_runtime_process_batch_tests.cpp
    dispatcher_runtime_process_tests.cpp
    dispatcher_runtime_skeleton_tests.cpp
    dispatcher_runtime_storage_tests.cpp
    dispatcher_runtime_summary_tests.cpp
    dispatcher_runtime_snapshot_tests.cpp
    runtime_health_tests.cpp
    runtime_diagnostics_tests.cpp
    runtime_consistency_tests.cpp
    runtime_hardening_tests.cpp
    runtime_production_hardening_header_tests.cpp
)

target_link_libraries(dispatcher-runtime-tests
    PRIVATE
        dispatcher-runtime
        GTest::gtest_main
)

add_test(NAME dispatcher-runtime-tests COMMAND dispatcher-runtime-tests)

add_executable(dispatcher-http-tests
    http_server_foundation_tests.cpp
    http_mapping_tests.cpp
    http_health_endpoint_tests.cpp
    http_runtime_alarm_endpoint_tests.cpp
    http_request_dispatcher_tests.cpp
    http_server_tests.cpp
    http_cors_tests.cpp
    http_json_tests.cpp
    http_runtime_contract_tests.cpp
    http_alarm_contract_tests.cpp
)

target_link_libraries(dispatcher-http-tests
    PRIVATE
        dispatcher-http
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-http-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-http-tests
    COMMAND dispatcher-http-tests
)

add_executable(dispatcher-storage-sqlite-tests
    sqlite_database_tests.cpp
    sqlite_migration_runner_tests.cpp
    sqlite_history_storage_tests.cpp
    sqlite_alarm_event_storage_tests.cpp
    sqlite_configuration_snapshot_storage_tests.cpp
    sqlite_storage_context_tests.cpp
)

target_link_libraries(dispatcher-storage-sqlite-tests
    PRIVATE
        dispatcher-storage-sqlite
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-storage-sqlite-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-storage-sqlite-tests
    COMMAND dispatcher-storage-sqlite-tests
)

add_executable(dispatcher-modbus-tests
    modbus_frame_codec_tests.cpp
    modbus_register_decoder_tests.cpp
    modbus_tcp_client_tests.cpp
    modbus_polling_config_tests.cpp
    modbus_telemetry_adapter_tests.cpp
    modbus_simulated_tcp_transport_tests.cpp
)

target_link_libraries(dispatcher-modbus-tests
    PRIVATE
        dispatcher-modbus
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-modbus-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-modbus-tests
    COMMAND dispatcher-modbus-tests
)

add_executable(dispatcher-notification-delivery-tests
    notification_delivery_foundation_tests.cpp
    notification_delivery_dispatcher_tests.cpp
    notification_delivery_retry_tests.cpp
    notification_delivery_file_channel_tests.cpp
    notification_delivery_webhook_channel_tests.cpp
    notification_delivery_alarm_integration_tests.cpp
)

target_link_libraries(dispatcher-notification-delivery-tests
    PRIVATE
        dispatcher-notification-delivery
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-notification-delivery-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-notification-delivery-tests
    COMMAND dispatcher-notification-delivery-tests
)

add_executable(dispatcher-auth-audit-tests
    auth_audit_foundation_tests.cpp
    auth_audit_logger_tests.cpp
    auth_authorization_policy_tests.cpp
    auth_authorization_audit_tests.cpp
    auth_http_adapter_tests.cpp
    auth_runtime_integration_tests.cpp
)

target_link_libraries(dispatcher-auth-audit-tests
    PRIVATE
        dispatcher-auth-audit
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(dispatcher-auth-audit-tests
    PRIVATE
        cxx_std_20
)

add_test(
    NAME dispatcher-auth-audit-tests
    COMMAND dispatcher-auth-audit-tests
)