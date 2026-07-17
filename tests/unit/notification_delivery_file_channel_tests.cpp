#include <dispatcher/notification/delivery/notification_delivery.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    std::filesystem::path test_root(
        const std::string& test_name
    )
    {
        return std::filesystem::temp_directory_path()
            / "dispatcher-notification-delivery-file-tests"
            / test_name;
    }

    void clean_directory(
        const std::filesystem::path& path
    )
    {
        std::error_code error_code;

        const auto removed =
            std::filesystem::remove_all(
                path,
                error_code
            );

        static_cast<void>(
            removed
            );
        static_cast<void>(
            error_code
            );
    }

    std::string read_text_file(
        const std::filesystem::path& path
    )
    {
        std::ifstream input{
            path
        };

        return std::string{
            std::istreambuf_iterator<char>{
                input
            },
            std::istreambuf_iterator<char>{}
        };
    }

    dispatcher::notification::delivery::NotificationDeliveryMessage make_file_message(
        std::string message_id = "message-1"
    )
    {
        dispatcher::notification::delivery::NotificationDeliveryMessage message;

        message.message_id =
            std::move(
                message_id
            );

        message.correlation_id = "alarm-1";
        message.source = "alarm-routing";
        message.channel_type =
            dispatcher::notification::delivery::NotificationDeliveryChannelType::file;
        message.priority =
            dispatcher::notification::delivery::NotificationDeliveryPriority::critical;

        message.recipient.recipient_id = "operator-1";
        message.recipient.display_name = "Operator One";
        message.recipient.address = "operator-1@example.local";

        message.subject = "Alarm raised";
        message.body = "Pump pressure is high.";

        message.attributes.emplace(
            "alarm_id",
            "alarm-1"
        );

        message.attributes.emplace(
            "tag_id",
            "pump.pressure"
        );

        return message;
    }

    dispatcher::notification::delivery::FileNotificationDeliveryOptions make_options(
        const std::string& test_name
    )
    {
        dispatcher::notification::delivery::FileNotificationDeliveryOptions options;

        options.directory =
            test_root(
                test_name
            );

        options.file_name =
            "notifications.log";

        options.create_directories = true;
        options.append = true;

        return options;
    }

    void expect_channel_construction_throws(
        const dispatcher::notification::delivery::FileNotificationDeliveryOptions& options
    )
    {
        EXPECT_THROW(
            {
                dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
                    options
                };

                static_cast<void>(
                    channel
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_deliver_throws(
        dispatcher::notification::delivery::FileNotificationDeliveryChannel& channel,
        const dispatcher::notification::delivery::NotificationDeliveryMessage& message
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    channel.deliver(
                        message
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }
}

TEST(NotificationDeliveryFileChannelTests, CreatesDirectoryAndWritesMessage)
{
    const auto root =
        test_root(
            "creates-directory-and-writes-message"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "creates-directory-and-writes-message"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    const auto result =
        channel.deliver(
            make_file_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_NE(
        result.provider_message_id.find(
            "message-1"
        ),
        std::string::npos
    );

    EXPECT_TRUE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_NE(
        content.find(
            "message_id=message-1"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "channel_type=file"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "priority=critical"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "attribute.alarm_id=alarm-1"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, AppendsMultipleMessages)
{
    const auto root =
        test_root(
            "appends-multiple-messages"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "appends-multiple-messages"
        );

    options.append = true;

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    const auto first =
        channel.deliver(
            make_file_message(
                "message-1"
            )
        );

    const auto second =
        channel.deliver(
            make_file_message(
                "message-2"
            )
        );

    EXPECT_TRUE(
        first.success()
    );

    EXPECT_TRUE(
        second.success()
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_NE(
        content.find(
            "message_id=message-1"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "message_id=message-2"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, OverwriteModeReplacesPreviousContent)
{
    const auto root =
        test_root(
            "overwrite-mode-replaces-previous-content"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "overwrite-mode-replaces-previous-content"
        );

    options.append = false;

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    const auto first =
        channel.deliver(
            make_file_message(
                "message-1"
            )
        );

    static_cast<void>(
        first
        );

    const auto second =
        channel.deliver(
            make_file_message(
                "message-2"
            )
        );

    EXPECT_TRUE(
        second.success()
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_EQ(
        content.find(
            "message_id=message-1"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "message_id=message-2"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, SkipsWrongChannelType)
{
    const auto root =
        test_root(
            "skips-wrong-channel-type"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "skips-wrong-channel-type"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    auto message =
        make_file_message();

    message.channel_type =
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test;

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_FALSE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, ReturnsFailedWhenDirectoryCannotBeCreated)
{
    const auto root =
        test_root(
            "returns-failed-when-directory-cannot-be-created"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "returns-failed-when-directory-cannot-be-created"
        );

    options.directory =
        root / "missing" / "nested";

    options.create_directories = false;

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    const auto result =
        channel.deliver(
            make_file_message()
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_NE(
        result.error_message.find(
            "Failed to open notification delivery file"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, RejectsEmptyDirectoryOption)
{
    auto options =
        make_options(
            "rejects-empty-directory-option"
        );

    options.directory =
        std::filesystem::path{};

    expect_channel_construction_throws(
        options
    );
}

TEST(NotificationDeliveryFileChannelTests, RejectsEmptyFileNameOption)
{
    auto options =
        make_options(
            "rejects-empty-file-name-option"
        );

    options.file_name = "";

    expect_channel_construction_throws(
        options
    );
}

TEST(NotificationDeliveryFileChannelTests, RejectsInvalidMessage)
{
    const auto root =
        test_root(
            "rejects-invalid-message"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "rejects-invalid-message"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    auto message =
        make_file_message();

    message.body = "";

    expect_deliver_throws(
        channel,
        message
    );

    EXPECT_FALSE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, SanitizesMultilineValues)
{
    const auto root =
        test_root(
            "sanitizes-multiline-values"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "sanitizes-multiline-values"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    auto message =
        make_file_message();

    message.subject =
        "Alarm\r\nraised";

    message.body =
        "Line 1\nLine 2";

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_TRUE(
        result.success()
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_NE(
        content.find(
            "subject=Alarm  raised"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "body=Line 1 Line 2"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, DispatcherDeliversThroughFileChannel)
{
    const auto root =
        test_root(
            "dispatcher-delivers-through-file-channel"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "dispatcher-delivers-through-file-channel"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher.deliver(
            make_file_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_TRUE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_NE(
        content.find(
            "message_id=message-1"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, RetryExecutorDeliversThroughFileChannel)
{
    const auto root =
        test_root(
            "retry-executor-delivers-through-file-channel"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "retry-executor-delivers-through-file-channel"
        );

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    const auto result =
        executor.deliver_with_retry(
            make_file_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.attempt_count(),
        1U
    );

    ASSERT_EQ(
        result.attempts.size(),
        1U
    );

    EXPECT_EQ(
        result.attempts[0].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_TRUE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryFileChannelTests, RetryExecutorRetriesFileOpenFailure)
{
    const auto root =
        test_root(
            "retry-executor-retries-file-open-failure"
        );

    clean_directory(
        root
    );

    auto options =
        make_options(
            "retry-executor-retries-file-open-failure"
        );

    options.directory =
        root / "missing" / "nested";

    options.create_directories = false;

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        executor.deliver_with_retry(
            make_file_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        2U
    );

    ASSERT_EQ(
        result.attempts.size(),
        2U
    );

    EXPECT_EQ(
        result.attempts[0].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_EQ(
        result.attempts[1].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    clean_directory(
        root
    );
}