#include <dispatcher/config/configuration_json_import_parser.hpp>

#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>

namespace
{
    using Json = nlohmann::json;

    std::string document_resource(
        const dispatcher::config::ConfigurationDocument& document
    )
    {
        if (document.has_name())
        {
            return document.name();
        }

        return {};
    }

    dispatcher::config::ConfigurationImportModelResult parse_failure(
        const dispatcher::config::ConfigurationDocument& document,
        std::string field,
        std::string message
    )
    {
        return dispatcher::config::ConfigurationImportModelResult::failure(
            dispatcher::config::ConfigurationIoStatus::ParseError,
            "configuration.import.parse",
            document_resource(document),
            std::move(field),
            std::move(message)
        );
    }

    dispatcher::config::ConfigurationImportModelResult unsupported_format(
        const dispatcher::config::ConfigurationDocument& document,
        std::string field,
        std::string message
    )
    {
        return dispatcher::config::ConfigurationImportModelResult::failure(
            dispatcher::config::ConfigurationIoStatus::UnsupportedFormat,
            "configuration.import.parse",
            document_resource(document),
            std::move(field),
            std::move(message)
        );
    }

    bool has_field(const Json& object, const char* field)
    {
        return object.find(field) != object.end();
    }

    dispatcher::config::ConfigurationImportModelResult read_optional_string(
        const dispatcher::config::ConfigurationDocument& document,
        const Json& object,
        const char* field,
        const std::string& field_path,
        std::string& target
    )
    {
        if (!has_field(object, field))
        {
            return dispatcher::config::ConfigurationImportModelResult::success(
                dispatcher::config::ConfigurationImportModel{}
            );
        }

        if (!object.at(field).is_string())
        {
            return parse_failure(
                document,
                field_path,
                "expected string"
            );
        }

        target = object.at(field).get<std::string>();

        return dispatcher::config::ConfigurationImportModelResult::success(
            dispatcher::config::ConfigurationImportModel{}
        );
    }

    dispatcher::config::ConfigurationImportModelResult read_optional_bool(
        const dispatcher::config::ConfigurationDocument& document,
        const Json& object,
        const char* field,
        const std::string& field_path,
        bool& target
    )
    {
        if (!has_field(object, field))
        {
            return dispatcher::config::ConfigurationImportModelResult::success(
                dispatcher::config::ConfigurationImportModel{}
            );
        }

        if (!object.at(field).is_boolean())
        {
            return parse_failure(
                document,
                field_path,
                "expected boolean"
            );
        }

        target = object.at(field).get<bool>();

        return dispatcher::config::ConfigurationImportModelResult::success(
            dispatcher::config::ConfigurationImportModel{}
        );
    }

    dispatcher::config::ConfigurationImportModelResult read_optional_uint64(
        const dispatcher::config::ConfigurationDocument& document,
        const Json& object,
        const char* field,
        const std::string& field_path,
        std::uint64_t& target
    )
    {
        if (!has_field(object, field))
        {
            return dispatcher::config::ConfigurationImportModelResult::success(
                dispatcher::config::ConfigurationImportModel{}
            );
        }

        if (!object.at(field).is_number_unsigned())
        {
            return parse_failure(
                document,
                field_path,
                "expected unsigned integer"
            );
        }

        target = object.at(field).get<std::uint64_t>();

        return dispatcher::config::ConfigurationImportModelResult::success(
            dispatcher::config::ConfigurationImportModel{}
        );
    }

    dispatcher::config::ConfigurationImportModelResult read_device(
        const dispatcher::config::ConfigurationDocument& document,
        const Json& value,
        std::size_t index,
        dispatcher::config::ConfigurationImportDevice& device
    )
    {
        if (!value.is_object())
        {
            return parse_failure(
                document,
                "devices[" + std::to_string(index) + "]",
                "expected object"
            );
        }

        const auto prefix = "devices[" + std::to_string(index) + "].";

        auto result = read_optional_string(
            document,
            value,
            "organization_id",
            prefix + "organization_id",
            device.organization_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "site_id",
            prefix + "site_id",
            device.site_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "area_id",
            prefix + "area_id",
            device.area_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "device_id",
            prefix + "device_id",
            device.device_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "local_name",
            prefix + "local_name",
            device.local_name
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "display_name",
            prefix + "display_name",
            device.display_name
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_bool(
            document,
            value,
            "enabled",
            prefix + "enabled",
            device.enabled
        );

        if (result.failed())
        {
            return result;
        }

        return dispatcher::config::ConfigurationImportModelResult::success(
            dispatcher::config::ConfigurationImportModel{}
        );
    }

    dispatcher::config::ConfigurationImportModelResult read_tag(
        const dispatcher::config::ConfigurationDocument& document,
        const Json& value,
        std::size_t index,
        dispatcher::config::ConfigurationImportTag& tag
    )
    {
        if (!value.is_object())
        {
            return parse_failure(
                document,
                "tags[" + std::to_string(index) + "]",
                "expected object"
            );
        }

        const auto prefix = "tags[" + std::to_string(index) + "].";

        auto result = read_optional_string(
            document,
            value,
            "organization_id",
            prefix + "organization_id",
            tag.organization_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "site_id",
            prefix + "site_id",
            tag.site_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "area_id",
            prefix + "area_id",
            tag.area_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "device_id",
            prefix + "device_id",
            tag.device_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "tag_id",
            prefix + "tag_id",
            tag.tag_id
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "local_name",
            prefix + "local_name",
            tag.local_name
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "display_name",
            prefix + "display_name",
            tag.display_name
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "data_type",
            prefix + "data_type",
            tag.data_type
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            value,
            "history_policy",
            prefix + "history_policy",
            tag.history_policy
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_bool(
            document,
            value,
            "enabled",
            prefix + "enabled",
            tag.enabled
        );

        if (result.failed())
        {
            return result;
        }

        return dispatcher::config::ConfigurationImportModelResult::success(
            dispatcher::config::ConfigurationImportModel{}
        );
    }
}

namespace dispatcher::config
{
    ConfigurationImportModelResult ConfigurationJsonImportParser::parse(
        const ConfigurationDocument& document
    )
    {
        if (document.format() != ConfigurationFormat::Json)
        {
            return unsupported_format(
                document,
                "document.format",
                "configuration document is not json"
            );
        }

        if (document.empty())
        {
            return parse_failure(
                document,
                "document.content",
                "configuration document is empty"
            );
        }

        Json json;

        try
        {
            json = Json::parse(document.content());
        }
        catch (const nlohmann::json::parse_error&)
        {
            return parse_failure(
                document,
                "document.content",
                "configuration document is not valid json"
            );
        }

        if (!json.is_object())
        {
            return parse_failure(
                document,
                "document",
                "configuration document root must be an object"
            );
        }

        ConfigurationImportMetadata metadata;

        auto result = read_optional_string(
            document,
            json,
            "schema_version",
            "schema_version",
            metadata.schema_version
        );

        if (result.failed())
        {
            return result;
        }

        std::string format_value{ "json" };

        result = read_optional_string(
            document,
            json,
            "format",
            "format",
            format_value
        );

        if (result.failed())
        {
            return result;
        }

        metadata.format = parse_configuration_format(format_value);

        if (!metadata.has_known_format())
        {
            return unsupported_format(
                document,
                "format",
                "unsupported configuration import format"
            );
        }

        result = read_optional_uint64(
            document,
            json,
            "config_version",
            "config_version",
            metadata.config_version
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            json,
            "status",
            "status",
            metadata.status
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            json,
            "description",
            "description",
            metadata.description
        );

        if (result.failed())
        {
            return result;
        }

        result = read_optional_string(
            document,
            json,
            "source",
            "source",
            metadata.source
        );

        if (result.failed())
        {
            return result;
        }

        if (metadata.source.empty() && document.has_name())
        {
            metadata.source = document.name();
        }

        result = read_optional_string(
            document,
            json,
            "imported_at",
            "imported_at",
            metadata.imported_at
        );

        if (result.failed())
        {
            return result;
        }

        if (metadata.imported_at.empty())
        {
            result = read_optional_string(
                document,
                json,
                "exported_at",
                "exported_at",
                metadata.imported_at
            );

            if (result.failed())
            {
                return result;
            }
        }

        ConfigurationImportModel model(metadata);

        if (has_field(json, "devices"))
        {
            if (!json.at("devices").is_array())
            {
                return parse_failure(
                    document,
                    "devices",
                    "expected array"
                );
            }

            const auto& devices = json.at("devices");

            for (std::size_t index = 0; index < devices.size(); ++index)
            {
                ConfigurationImportDevice device;

                result = read_device(
                    document,
                    devices.at(index),
                    index,
                    device
                );

                if (result.failed())
                {
                    return result;
                }

                model.add_device(std::move(device));
            }
        }

        if (has_field(json, "tags"))
        {
            if (!json.at("tags").is_array())
            {
                return parse_failure(
                    document,
                    "tags",
                    "expected array"
                );
            }

            const auto& tags = json.at("tags");

            for (std::size_t index = 0; index < tags.size(); ++index)
            {
                ConfigurationImportTag tag;

                result = read_tag(
                    document,
                    tags.at(index),
                    index,
                    tag
                );

                if (result.failed())
                {
                    return result;
                }

                model.add_tag(std::move(tag));
            }
        }

        return ConfigurationImportModelResult::success(
            std::move(model)
        );
    }
}