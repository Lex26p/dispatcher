#include <dispatcher/notification/delivery/file_notification_delivery_channel.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>
#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <fstream>
#include <ios>
#include <utility>

namespace dispatcher::notification::delivery
{
    FileNotificationDeliveryChannel::FileNotificationDeliveryChannel(
        FileNotificationDeliveryOptions options
    )
        : options_(
            std::move(
                options
            )
        )
    {
        validate_options(
            options_
        );
    }

    NotificationDeliveryChannelType FileNotificationDeliveryChannel::channel_type() const noexcept
    {
        return NotificationDeliveryChannelType::file;
    }

    std::string FileNotificationDeliveryChannel::channel_name() const
    {
        return "file-notification-delivery";
    }

    NotificationDeliveryResult FileNotificationDeliveryChannel::deliver(
        const NotificationDeliveryMessage& message
    )
    {
        NotificationDeliveryValidator::validate_message(
            message
        );

        if (message.channel_type != NotificationDeliveryChannelType::file)
        {
            return NotificationDeliveryResult::skipped(
                "Message channel_type does not match file notification delivery channel."
            );
        }

        const auto path =
            output_path();

        if (options_.create_directories)
        {
            std::error_code error_code;

            const auto created =
                std::filesystem::create_directories(
                    options_.directory,
                    error_code
                );

            static_cast<void>(
                created
                );

            if (error_code)
            {
                return NotificationDeliveryResult::failed(
                    "Failed to create notification delivery directory: "
                    + error_code.message(),
                    path.string()
                );
            }
        }

        auto open_mode =
            std::ios::out;

        if (options_.append)
        {
            open_mode |= std::ios::app;
        }
        else
        {
            open_mode |= std::ios::trunc;
        }

        std::ofstream output{
            path,
            open_mode
        };

        if (!output.is_open())
        {
            return NotificationDeliveryResult::failed(
                "Failed to open notification delivery file.",
                path.string()
            );
        }

        output
            << "--- notification ---\n"
            << "message_id=" << sanitize_line_value(
                message.message_id
            ) << "\n"
            << "correlation_id=" << sanitize_line_value(
                message.correlation_id
            ) << "\n"
            << "source=" << sanitize_line_value(
                message.source
            ) << "\n"
            << "channel_type="
            << NotificationDeliveryValidator::channel_type_to_string(
                message.channel_type
            ) << "\n"
            << "priority="
            << NotificationDeliveryValidator::priority_to_string(
                message.priority
            ) << "\n"
            << "recipient_id=" << sanitize_line_value(
                message.recipient.recipient_id
            ) << "\n"
            << "recipient_display_name=" << sanitize_line_value(
                message.recipient.display_name
            ) << "\n"
            << "recipient_address=" << sanitize_line_value(
                message.recipient.address
            ) << "\n"
            << "subject=" << sanitize_line_value(
                message.subject
            ) << "\n"
            << "body=" << sanitize_line_value(
                message.body
            ) << "\n";

        for (const auto& [key, value] : message.attributes)
        {
            output
                << "attribute."
                << sanitize_line_value(
                    key
                )
                << "="
                << sanitize_line_value(
                    value
                )
                << "\n";
        }

        output
            << "--- end notification ---\n";

        if (!output.good())
        {
            return NotificationDeliveryResult::failed(
                "Failed to write notification delivery file.",
                path.string()
            );
        }

        return NotificationDeliveryResult::delivered(
            make_provider_message_id(
                path,
                message
            ),
            "Delivered by file notification delivery channel."
        );
    }

    const FileNotificationDeliveryOptions& FileNotificationDeliveryChannel::options() const noexcept
    {
        return options_;
    }

    std::filesystem::path FileNotificationDeliveryChannel::output_path() const
    {
        return options_.directory / options_.file_name;
    }

    void FileNotificationDeliveryChannel::validate_options(
        const FileNotificationDeliveryOptions& options
    )
    {
        if (options.directory.empty())
        {
            throw NotificationDeliveryError(
                "File notification delivery directory must not be empty."
            );
        }

        if (options.file_name.empty())
        {
            throw NotificationDeliveryError(
                "File notification delivery file_name must not be empty."
            );
        }
    }

    std::string FileNotificationDeliveryChannel::sanitize_line_value(
        const std::string& value
    )
    {
        std::string sanitized;

        sanitized.reserve(
            value.size()
        );

        for (const auto character : value)
        {
            if (character == '\r' || character == '\n')
            {
                sanitized.push_back(
                    ' '
                );

                continue;
            }

            sanitized.push_back(
                character
            );
        }

        return sanitized;
    }

    std::string FileNotificationDeliveryChannel::make_provider_message_id(
        const std::filesystem::path& path,
        const NotificationDeliveryMessage& message
    )
    {
        return path.string()
            + ":"
            + message.message_id;
    }
}