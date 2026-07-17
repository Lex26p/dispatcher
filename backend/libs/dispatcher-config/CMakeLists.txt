find_package(nlohmann_json CONFIG REQUIRED)

add_library(dispatcher-config
    src/configuration_format.cpp
    src/configuration_io_status.cpp
    src/configuration_io_result.cpp
    src/configuration_document.cpp
    src/configuration_document_result.cpp
    src/configuration_export_metadata.cpp
    src/configuration_export_device.cpp
    src/configuration_export_tag.cpp
    src/configuration_export_model.cpp
    src/configuration_export_model_result.cpp
    src/configuration_export_options.cpp
    src/configuration_snapshot_export_mapper.cpp
    src/configuration_json_export_serializer.cpp
    src/configuration_exporter.cpp
    src/configuration_import_metadata.cpp
    src/configuration_import_device.cpp
    src/configuration_import_tag.cpp
    src/configuration_import_model.cpp
    src/configuration_import_model_result.cpp
    src/configuration_snapshot_import_result.cpp
    src/configuration_import_model_mapper.cpp
    src/configuration_json_import_parser.cpp
    src/configuration_importer.cpp
)

target_include_directories(dispatcher-config
    PUBLIC
        include
)

target_link_libraries(dispatcher-config
    PUBLIC
        dispatcher-common
        dispatcher-domain
        dispatcher-alarm
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_compile_features(dispatcher-config PUBLIC cxx_std_20)