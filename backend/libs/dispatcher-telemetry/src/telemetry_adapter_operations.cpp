#include <dispatcher/telemetry/telemetry_adapter_operations.hpp>

#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>

#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_placeholder_value(
        const dispatcher::domain::TagId& tag_id
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ tag_id.value() },
            TagValue(0.0),
            Quality::Good,
            now,
            now,
            0
        );
    }
}

namespace dispatcher::telemetry
{
    TelemetryReadResult read_from_adapter(
        TelemetryAdapter* adapter,
        const TelemetryReadRequest& request
    )
    {
        if (adapter == nullptr)
        {
            return TelemetryReadResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_operations.read",
                "telemetry-adapter-operations",
                {},
                "adapter",
                "adapter pointer is null"
            );
        }

        if (request.empty())
        {
            return TelemetryReadResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_operations.read",
                adapter->name(),
                {},
                "request",
                "read request is empty"
            );
        }

        if (!adapter->capabilities().can_read_current())
        {
            return TelemetryReadResult::failure(
                TelemetryAdapterStatus::UnsupportedOperation,
                "telemetry_adapter_operations.read",
                adapter->name(),
                {},
                "capability",
                "adapter does not support read_current"
            );
        }

        if (request.single())
        {
            auto value = make_placeholder_value(
                request.tag_ids().front()
            );

            const auto result = adapter->read_current(
                request.tag_ids().front(),
                value
            );

            if (result.failed())
            {
                return TelemetryReadResult::from_adapter_result(result);
            }

            return TelemetryReadResult::success(
                std::move(value)
            );
        }

        std::vector<TelemetryValue> values;

        const auto result = adapter->read_current_batch(
            request.tag_ids(),
            values
        );

        if (result.failed())
        {
            return TelemetryReadResult::from_adapter_result(result);
        }

        return TelemetryReadResult::success(
            std::move(values)
        );
    }

    TelemetryReadResult read_from_adapter(
        TelemetryAdapter& adapter,
        const TelemetryReadRequest& request
    )
    {
        return read_from_adapter(
            &adapter,
            request
        );
    }

    TelemetryWriteResult write_to_adapter(
        TelemetryAdapter* adapter,
        const TelemetryWriteRequest& request
    )
    {
        if (adapter == nullptr)
        {
            return TelemetryWriteResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_operations.write",
                "telemetry-adapter-operations",
                {},
                "adapter",
                "adapter pointer is null"
            );
        }

        if (request.empty())
        {
            return TelemetryWriteResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_operations.write",
                adapter->name(),
                {},
                "request",
                "write request is empty"
            );
        }

        if (!adapter->capabilities().can_write_current())
        {
            return TelemetryWriteResult::failure(
                TelemetryAdapterStatus::UnsupportedOperation,
                "telemetry_adapter_operations.write",
                adapter->name(),
                {},
                "capability",
                "adapter does not support write_current"
            );
        }

        std::size_t written_count = 0;

        for (const auto& value : request.values())
        {
            const auto result = adapter->write_current(
                value.tag_id(),
                value
            );

            if (result.failed())
            {
                return TelemetryWriteResult::from_adapter_result(
                    result,
                    written_count
                );
            }

            ++written_count;
        }

        return TelemetryWriteResult::success(written_count);
    }

    TelemetryWriteResult write_to_adapter(
        TelemetryAdapter& adapter,
        const TelemetryWriteRequest& request
    )
    {
        return write_to_adapter(
            &adapter,
            request
        );
    }
}