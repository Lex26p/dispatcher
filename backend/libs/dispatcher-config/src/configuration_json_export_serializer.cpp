#include <dispatcher/config/configuration_json_export_serializer.hpp>

#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    std::string escape_json_string(std::string_view value)
    {
        std::string result;
        result.reserve(value.size());

        constexpr char hex[] = "0123456789abcdef";

        for (const unsigned char character : value)
        {
            switch (character)
            {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:
                if (character < 0x20)
                {
                    result += "\\u00";
                    result += hex[(character >> 4) & 0x0F];
                    result += hex[character & 0x0F];
                }
                else
                {
                    result.push_back(static_cast<char>(character));
                }

                break;
            }
        }

        return result;
    }

    void append_indent(std::ostringstream& output, int spaces)
    {
        for (int index = 0; index < spaces; ++index)
        {
            output << ' ';
        }
    }

    void append_string_field(
        std::ostringstream& output,
        int indent,
        std::string_view name,
        std::string_view value,
        bool comma
    )
    {
        append_indent(output, indent);

        output
            << '"'
            << name
            << "\": \""
            << escape_json_string(value)
            << '"';

        if (comma)
        {
            output << ',';
        }

        output << '\n';
    }

    void append_uint64_field(
        std::ostringstream& output,
        int indent,
        std::string_view name,
        std::uint64_t value,
        bool comma
    )
    {
        append_indent(output, indent);

        output
            << '"'
            << name
            << "\": "
            << value;

        if (comma)
        {
            output << ',';
        }

        output << '\n';
    }

    void append_bool_field(
        std::ostringstream& output,
        int indent,
        std::string_view name,
        bool value,
        bool comma
    )
    {
        append_indent(output, indent);

        output
            << '"'
            << name
            << "\": "
            << (value ? "true" : "false");

        if (comma)
        {
            output << ',';
        }

        output << '\n';
    }

    void append_device(
        std::ostringstream& output,
        const dispatcher::config::ConfigurationExportDevice& device,
        bool comma
    )
    {
        append_indent(output, 4);
        output << "{\n";

        append_string_field(
            output,
            6,
            "organization_id",
            device.organization_id,
            true
        );

        append_string_field(output, 6, "site_id", device.site_id, true);
        append_string_field(output, 6, "area_id", device.area_id, true);
        append_string_field(output, 6, "device_id", device.device_id, true);
        append_string_field(output, 6, "local_name", device.local_name, true);

        append_string_field(
            output,
            6,
            "display_name",
            device.display_name,
            true
        );

        append_bool_field(output, 6, "enabled", device.enabled, false);

        append_indent(output, 4);
        output << '}';

        if (comma)
        {
            output << ',';
        }

        output << '\n';
    }

    void append_tag(
        std::ostringstream& output,
        const dispatcher::config::ConfigurationExportTag& tag,
        bool comma
    )
    {
        append_indent(output, 4);
        output << "{\n";

        append_string_field(
            output,
            6,
            "organization_id",
            tag.organization_id,
            true
        );

        append_string_field(output, 6, "site_id", tag.site_id, true);
        append_string_field(output, 6, "area_id", tag.area_id, true);
        append_string_field(output, 6, "device_id", tag.device_id, true);
        append_string_field(output, 6, "tag_id", tag.tag_id, true);
        append_string_field(output, 6, "local_name", tag.local_name, true);

        append_string_field(
            output,
            6,
            "display_name",
            tag.display_name,
            true
        );

        append_string_field(output, 6, "data_type", tag.data_type, true);

        append_string_field(
            output,
            6,
            "history_policy",
            tag.history_policy,
            true
        );

        append_bool_field(output, 6, "enabled", tag.enabled, false);

        append_indent(output, 4);
        output << '}';

        if (comma)
        {
            output << ',';
        }

        output << '\n';
    }

    void append_devices(
        std::ostringstream& output,
        const std::vector<dispatcher::config::ConfigurationExportDevice>& devices
    )
    {
        append_indent(output, 2);
        output << "\"devices\": ";

        if (devices.empty())
        {
            output << "[],\n";
            return;
        }

        output << "[\n";

        for (std::size_t index = 0; index < devices.size(); ++index)
        {
            append_device(
                output,
                devices[index],
                index + 1 < devices.size()
            );
        }

        append_indent(output, 2);
        output << "],\n";
    }

    void append_tags(
        std::ostringstream& output,
        const std::vector<dispatcher::config::ConfigurationExportTag>& tags
    )
    {
        append_indent(output, 2);
        output << "\"tags\": ";

        if (tags.empty())
        {
            output << "[]\n";
            return;
        }

        output << "[\n";

        for (std::size_t index = 0; index < tags.size(); ++index)
        {
            append_tag(
                output,
                tags[index],
                index + 1 < tags.size()
            );
        }

        append_indent(output, 2);
        output << "]\n";
    }
}

namespace dispatcher::config
{
    ConfigurationDocumentResult ConfigurationJsonExportSerializer::serialize(
        const ConfigurationExportModel& model,
        std::string name
    )
    {
        if (model.metadata().format != ConfigurationFormat::Json)
        {
            return ConfigurationDocumentResult::failure(
                ConfigurationIoStatus::UnsupportedFormat,
                "configuration.export.serialize",
                std::to_string(model.metadata().config_version),
                "metadata.format",
                "configuration export model is not json"
            );
        }

        std::ostringstream output;

        output << "{\n";

        append_string_field(
            output,
            2,
            "schema_version",
            model.metadata().schema_version,
            true
        );

        append_string_field(
            output,
            2,
            "format",
            to_string(model.metadata().format),
            true
        );

        append_uint64_field(
            output,
            2,
            "config_version",
            model.metadata().config_version,
            true
        );

        append_string_field(
            output,
            2,
            "status",
            model.metadata().status,
            true
        );

        append_string_field(
            output,
            2,
            "description",
            model.metadata().description,
            true
        );

        append_string_field(
            output,
            2,
            "source",
            model.metadata().source,
            true
        );

        append_string_field(
            output,
            2,
            "exported_at",
            model.metadata().exported_at,
            true
        );

        append_devices(output, model.devices());
        append_tags(output, model.tags());

        output << "}\n";

        return ConfigurationDocumentResult::success(
            ConfigurationDocument::json(
                output.str(),
                std::move(name)
            )
        );
    }
}