#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_adapter.hpp>
#include <dispatcher/telemetry/telemetry_adapter_capability.hpp>
#include <dispatcher/telemetry/telemetry_adapter_operations.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_read_request.hpp>
#include <dispatcher/telemetry/telemetry_read_result.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>
#include <dispatcher/telemetry/telemetry_write_request.hpp>
#include <dispatcher/telemetry/telemetry_write_result.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_operations_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        std::uint64_t sequence = 1
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }

    class OperationsTelemetryAdapter final
        : public dispatcher::telemetry::TelemetryAdapter
    {
    public:
        explicit OperationsTelemetryAdapter(
            std::string name,
            dispatcher::telemetry::TelemetryAdapterCapabilities capabilities =
            dispatcher::telemetry::TelemetryAdapterCapabilities::all()
        )
            : name_(std::move(name))
            , capabilities_(capabilities)
        {
        }

        std::string name() const override
        {
            return name_;
        }

        dispatcher::telemetry::TelemetryAdapterCapabilities capabilities()
            const noexcept override
        {
            return capabilities_;
        }

        bool connected() const noexcept override
        {
            return connected_;
        }

        dispatcher::telemetry::TelemetryAdapterResult connect() override
        {
            connected_ = true;

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult disconnect() override
        {
            connected_ = false;

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult health_check() override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.health_check",
                    name_,
                    {},
                    "connected",
                    "adapter is not connected"
                );
            }

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult read_current(
            const dispatcher::domain::TagId& tag_id,
            dispatcher::telemetry::TelemetryValue& value
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.read_current",
                    name_,
                    tag_id.value(),
                    "connected",
                    "adapter is not connected"
                );
            }

            if (tag_id.value() == "tag-missing")
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag,
                    "adapter.read_current",
                    name_,
                    tag_id.value(),
                    "tag_id",
                    "tag id is not configured"
                );
            }

            value = make_operations_value(
                tag_id.value(),
                dispatcher::telemetry::TagValue(42.0),
                ++sequence_
            );

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult read_current_batch(
            const std::vector<dispatcher::domain::TagId>& tag_ids,
            std::vector<dispatcher::telemetry::TelemetryValue>& values
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.read_current_batch",
                    name_,
                    {},
                    "connected",
                    "adapter is not connected"
                );
            }

            values.clear();
            values.reserve(tag_ids.size());

            for (const auto& tag_id : tag_ids)
            {
                if (tag_id.value() == "tag-missing")
                {
                    return dispatcher::telemetry::TelemetryAdapterResult::failure(
                        dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag,
                        "adapter.read_current_batch",
                        name_,
                        tag_id.value(),
                        "tag_id",
                        "tag id is not configured"
                    );
                }

                values.push_back(
                    make_operations_value(
                        tag_id.value(),
                        dispatcher::telemetry::TagValue(42.0),
                        ++sequence_
                    )
                );
            }

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult write_current(
            const dispatcher::domain::TagId& tag_id,
            const dispatcher::telemetry::TelemetryValue& value
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.write_current",
                    name_,
                    tag_id.value(),
                    "connected",
                    "adapter is not connected"
                );
            }

            if (tag_id.value() == "tag-fail")
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::IoError,
                    "adapter.write_current",
                    name_,
                    tag_id.value(),
                    "io",
                    "write failed"
                );
            }

            written_tag_ids_.push_back(tag_id.value());
            last_written_sequence_ = value.sequence();

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        [[nodiscard]] const std::vector<std::string>& written_tag_ids()
            const noexcept
        {
            return written_tag_ids_;
        }

        [[nodiscard]] std::uint64_t last_written_sequence() const noexcept
        {
            return last_written_sequence_;
        }

    private:
        std::string name_;
        dispatcher::telemetry::TelemetryAdapterCapabilities capabilities_;
        bool connected_{ false };
        std::uint64_t sequence_{ 0 };
        std::vector<std::string> written_tag_ids_;
        std::uint64_t last_written_sequence_{ 0 };
    };
}

TEST(TelemetryAdapterOperationsTests, ReadRejectsNullAdapter)
{
    const auto result =
        dispatcher::telemetry::read_from_adapter(
            nullptr,
            dispatcher::telemetry::TelemetryReadRequest::single(
                dispatcher::domain::TagId{ "tag-temperature" }
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "telemetry_adapter_operations.read");
    EXPECT_EQ(result.error().field(), "adapter");
}

TEST(TelemetryAdapterOperationsTests, ReadRejectsEmptyRequest)
{
    OperationsTelemetryAdapter adapter("simulator");

    const dispatcher::telemetry::TelemetryReadRequest request;

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            request
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().field(), "request");
    EXPECT_EQ(result.error().message(), "read request is empty");
}

TEST(TelemetryAdapterOperationsTests, ReadRejectsUnsupportedCapability)
{
    OperationsTelemetryAdapter adapter(
        "write-only",
        dispatcher::telemetry::TelemetryAdapterCapabilities{
            dispatcher::telemetry::TelemetryAdapterCapability::Connect,
            dispatcher::telemetry::TelemetryAdapterCapability::WriteCurrent
        }
    );

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            dispatcher::telemetry::TelemetryReadRequest::single(
                dispatcher::domain::TagId{ "tag-temperature" }
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().field(), "capability");
    EXPECT_EQ(
        result.error().message(),
        "adapter does not support read_current"
    );
}

TEST(TelemetryAdapterOperationsTests, ReadReturnsAdapterFailureWhenDisconnected)
{
    OperationsTelemetryAdapter adapter("simulator");

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            dispatcher::telemetry::TelemetryReadRequest::single(
                dispatcher::domain::TagId{ "tag-temperature" }
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.read_current");
    EXPECT_TRUE(result.retryable());
}

TEST(TelemetryAdapterOperationsTests, ReadSingleReturnsOneTelemetryValue)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            dispatcher::telemetry::TelemetryReadRequest::single(
                dispatcher::domain::TagId{ "tag-temperature" }
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_TRUE(result.single());
    EXPECT_EQ(result.size(), 1);

    EXPECT_EQ(
        result.value().tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(result.value().sequence(), 1);
}

TEST(TelemetryAdapterOperationsTests, ReadBatchReturnsTelemetryValues)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            dispatcher::telemetry::TelemetryReadRequest::batch(
                {
                    dispatcher::domain::TagId{"tag-temperature"},
                    dispatcher::domain::TagId{"tag-pressure"}
                }
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_TRUE(result.batch());
    ASSERT_EQ(result.size(), 2);

    EXPECT_EQ(
        result.values()[0].tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(
        result.values()[1].tag_id(),
        dispatcher::domain::TagId{ "tag-pressure" }
    );

    EXPECT_EQ(result.values()[0].sequence(), 1);
    EXPECT_EQ(result.values()[1].sequence(), 2);
}

TEST(TelemetryAdapterOperationsTests, ReadBatchReturnsAdapterFailure)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::read_from_adapter(
            adapter,
            dispatcher::telemetry::TelemetryReadRequest::batch(
                {
                    dispatcher::domain::TagId{"tag-temperature"},
                    dispatcher::domain::TagId{"tag-missing"}
                }
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.read_current_batch");
    EXPECT_EQ(result.error().resource(), "tag-missing");
}

TEST(TelemetryAdapterOperationsTests, WriteRejectsNullAdapter)
{
    const auto result =
        dispatcher::telemetry::write_to_adapter(
            nullptr,
            dispatcher::telemetry::TelemetryWriteRequest::single(
                make_operations_value(
                    "tag-temperature",
                    dispatcher::telemetry::TagValue(42.0),
                    1
                )
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "telemetry_adapter_operations.write");
    EXPECT_EQ(result.error().field(), "adapter");
}

TEST(TelemetryAdapterOperationsTests, WriteRejectsEmptyRequest)
{
    OperationsTelemetryAdapter adapter("simulator");

    const dispatcher::telemetry::TelemetryWriteRequest request;

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            request
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().field(), "request");
    EXPECT_EQ(result.error().message(), "write request is empty");
}

TEST(TelemetryAdapterOperationsTests, WriteRejectsUnsupportedCapability)
{
    OperationsTelemetryAdapter adapter(
        "read-only",
        dispatcher::telemetry::TelemetryAdapterCapabilities{
            dispatcher::telemetry::TelemetryAdapterCapability::Connect,
            dispatcher::telemetry::TelemetryAdapterCapability::ReadCurrent
        }
    );

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            dispatcher::telemetry::TelemetryWriteRequest::single(
                make_operations_value(
                    "tag-temperature",
                    dispatcher::telemetry::TagValue(42.0),
                    1
                )
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().field(), "capability");
    EXPECT_EQ(
        result.error().message(),
        "adapter does not support write_current"
    );
}

TEST(TelemetryAdapterOperationsTests, WriteReturnsAdapterFailureWhenDisconnected)
{
    OperationsTelemetryAdapter adapter("simulator");

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            dispatcher::telemetry::TelemetryWriteRequest::single(
                make_operations_value(
                    "tag-temperature",
                    dispatcher::telemetry::TagValue(42.0),
                    1
                )
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.write_current");
    EXPECT_TRUE(result.retryable());
}

TEST(TelemetryAdapterOperationsTests, WriteSingleReturnsWrittenCount)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            dispatcher::telemetry::TelemetryWriteRequest::single(
                make_operations_value(
                    "tag-temperature",
                    dispatcher::telemetry::TagValue(42.0),
                    10
                )
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(result.written_count(), 1);
    EXPECT_TRUE(result.wrote_expected(1));

    ASSERT_EQ(adapter.written_tag_ids().size(), 1);
    EXPECT_EQ(adapter.written_tag_ids().front(), "tag-temperature");
    EXPECT_EQ(adapter.last_written_sequence(), 10);
}

TEST(TelemetryAdapterOperationsTests, WriteBatchReturnsWrittenCount)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_operations_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(42.0),
            1
        )
    );

    values.push_back(
        make_operations_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(10.0),
            2
        )
    );

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            dispatcher::telemetry::TelemetryWriteRequest::batch(
                std::move(values)
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(result.written_count(), 2);
    EXPECT_TRUE(result.wrote_expected(2));

    ASSERT_EQ(adapter.written_tag_ids().size(), 2);
    EXPECT_EQ(adapter.written_tag_ids()[0], "tag-temperature");
    EXPECT_EQ(adapter.written_tag_ids()[1], "tag-pressure");
}

TEST(TelemetryAdapterOperationsTests, WriteBatchKeepsPartialWrittenCountOnFailure)
{
    OperationsTelemetryAdapter adapter("simulator");

    ASSERT_TRUE(adapter.connect().ok());

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_operations_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(42.0),
            1
        )
    );

    values.push_back(
        make_operations_value(
            "tag-fail",
            dispatcher::telemetry::TagValue(10.0),
            2
        )
    );

    values.push_back(
        make_operations_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(12.0),
            3
        )
    );

    const auto result =
        dispatcher::telemetry::write_to_adapter(
            adapter,
            dispatcher::telemetry::TelemetryWriteRequest::batch(
                std::move(values)
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::IoError
    );

    EXPECT_EQ(result.written_count(), 1);
    EXPECT_TRUE(result.wrote_any());

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.write_current");
    EXPECT_EQ(result.error().resource(), "tag-fail");

    ASSERT_EQ(adapter.written_tag_ids().size(), 1);
    EXPECT_EQ(adapter.written_tag_ids().front(), "tag-temperature");
}