#include <dispatcher/core/telemetry_ingestor.hpp>

#include <dispatcher/core/telemetry_ingest_batch_result.hpp>
#include <dispatcher/core/telemetry_ingest_result.hpp>
#include <dispatcher/domain/configuration_snapshot_validation.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/quality.hpp>

#include <utility>
namespace
{
    bool is_rejected_quality(dispatcher::domain::Quality quality)
    {
        using dispatcher::domain::Quality;

        switch (quality)
        {
        case Quality::Bad:
        case Quality::Timeout:
        case Quality::CommunicationError:
        case Quality::Disabled:
            return true;

        case Quality::Good:
        case Quality::Uncertain:
        case Quality::Stale:
        case Quality::Substituted:
        case Quality::ManualOverride:
            return false;
        }

        return true;
    }

    bool is_float64_value(
        const dispatcher::telemetry::TelemetryValue& value
    )
    {
        return value.value().type() == dispatcher::domain::DataType::Float64;
    }

    double as_float64(
        const dispatcher::telemetry::TelemetryValue& value
    )
    {
        return value.value().as<double>();
    }
}

namespace dispatcher::core
{
    TelemetryIngestor::TelemetryIngestor(
        dispatcher::domain::ConfigurationSnapshot configuration_snapshot
    )
        : TelemetryIngestor(
            std::move(configuration_snapshot),
            std::chrono::milliseconds{ 5000 }
        )
    {
    }

    TelemetryIngestor::TelemetryIngestor(
        dispatcher::domain::ConfigurationSnapshot configuration_snapshot,
        std::chrono::milliseconds max_future_source_timestamp_skew
    )
        : configuration_snapshot_(std::move(configuration_snapshot))
        , max_future_source_timestamp_skew_(max_future_source_timestamp_skew)
    {
    }

    const dispatcher::domain::ConfigurationSnapshot& TelemetryIngestor::configuration_snapshot()
        const noexcept
    {
        return configuration_snapshot_;
    }

    const CurrentStateStore& TelemetryIngestor::current_state() const noexcept
    {
        return current_state_;
    }

    const TelemetryIngestStatistics& TelemetryIngestor::statistics() const noexcept
    {
        return statistics_;
    }

    TelemetryRuntimeSnapshot TelemetryIngestor::runtime_snapshot() const noexcept
    {
        return TelemetryRuntimeSnapshot{
            .configuration_version = configuration_snapshot_.config_version(),
            .current_state_size = current_state_.size(),

            .accepted_count = statistics_.accepted_count(),
            .stored_count = statistics_.stored_count(),
            .accepted_no_change_count = statistics_.accepted_no_change_count(),

            .rejected_count = statistics_.rejected_count(),
            .unknown_tag_count = statistics_.unknown_tag_count(),
            .disabled_tag_count = statistics_.disabled_tag_count(),
            .data_type_mismatch_count = statistics_.data_type_mismatch_count(),
            .stale_sequence_count = statistics_.stale_sequence_count(),
            .future_source_timestamp_count = statistics_.future_source_timestamp_count(),
            .bad_quality_count = statistics_.bad_quality_count(),

            .total_count = statistics_.total_count()
        };
    }

    std::chrono::milliseconds TelemetryIngestor::max_future_source_timestamp_skew()
        const noexcept
    {
        return max_future_source_timestamp_skew_;
    }

    std::optional<std::uint64_t> TelemetryIngestor::last_sequence(
        const dispatcher::domain::TagId& tag_id
    ) const
    {
        const auto iterator = last_sequence_by_tag_id_.find(tag_id.value());

        if (iterator == last_sequence_by_tag_id_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    dispatcher::common::ValidationResult TelemetryIngestor::reload_configuration(
        dispatcher::domain::ConfigurationSnapshot configuration_snapshot
    )
    {
        auto result = dispatcher::domain::validate_configuration_snapshot(
            configuration_snapshot
        );

        if (configuration_snapshot.is_draft())
        {
            result.add_error(
                "configuration.status",
                "only published configuration snapshots can be loaded"
            );
        }

        if (result.has_errors())
        {
            return result;
        }

        configuration_snapshot_ = std::move(configuration_snapshot);

        return result;
    }

    void TelemetryIngestor::reset_statistics() noexcept
    {
        statistics_.reset();
    }

    bool TelemetryIngestor::is_future_source_timestamp(
        const dispatcher::telemetry::TelemetryValue& value
    ) const
    {
        const auto now = dispatcher::telemetry::TelemetryValue::Clock::now();

        return value.source_timestamp()
            > now + max_future_source_timestamp_skew_;
    }

    void TelemetryIngestor::record_last_sequence(
        const dispatcher::domain::TagId& tag_id,
        std::uint64_t sequence
    )
    {
        last_sequence_by_tag_id_[tag_id.value()] = sequence;
    }

    TelemetryIngestResult TelemetryIngestor::ingest(
        dispatcher::telemetry::TelemetryValue value
    )
    {
        const auto tag = configuration_snapshot_.find_tag_by_id(value.tag_id());

        if (!tag.has_value())
        {
            statistics_.record_unknown_tag();

            return TelemetryIngestResult(
                TelemetryIngestStatus::UnknownTag,
                "unknown tag: " + value.tag_id().value()
            );
        }

        if (!tag->enabled())
        {
            statistics_.record_disabled_tag();

            return TelemetryIngestResult(
                TelemetryIngestStatus::DisabledTag,
                "tag is disabled: " + value.tag_id().value()
            );
        }

        if (tag->data_type() != value.value().type())
        {
            statistics_.record_data_type_mismatch();

            return TelemetryIngestResult(
                TelemetryIngestStatus::DataTypeMismatch,
                "telemetry value type does not match tag definition: " + value.tag_id().value()
            );
        }

        if (is_future_source_timestamp(value))
        {
            statistics_.record_future_source_timestamp();

            return TelemetryIngestResult(
                TelemetryIngestStatus::FutureSourceTimestamp,
                "telemetry value source timestamp is too far in the future: "
                + value.tag_id().value()
            );
        }

        if (is_rejected_quality(value.quality()))
        {
            statistics_.record_bad_quality();

            return TelemetryIngestResult(
                TelemetryIngestStatus::BadQuality,
                "telemetry value quality is rejected: " + value.tag_id().value()
            );
        }

        const auto previous_sequence = last_sequence(value.tag_id());

        if (previous_sequence.has_value() && value.sequence() <= previous_sequence.value())
        {
            statistics_.record_stale_sequence();

            return TelemetryIngestResult(
                TelemetryIngestStatus::StaleSequence,
                "telemetry value sequence is stale: " + value.tag_id().value()
            );
        }

        const auto previous_value = current_state_.find(value.tag_id());

        if (previous_value.has_value()
            && is_float64_value(*previous_value)
            && is_float64_value(value)
            && !tag->deadband().accepts_change(
                as_float64(previous_value.value()),
                as_float64(value)
            ))
        {
            record_last_sequence(value.tag_id(), value.sequence());
            statistics_.record_accepted_no_change();

            return TelemetryIngestResult(
                TelemetryIngestStatus::AcceptedNoChange,
                "telemetry value accepted but ignored by deadband: " + value.tag_id().value()
            );
        }

        record_last_sequence(value.tag_id(), value.sequence());
        current_state_.update(std::move(value));
        statistics_.record_accepted_stored();

        return TelemetryIngestResult(
            TelemetryIngestStatus::Accepted
        );
    }

    TelemetryIngestBatchResult TelemetryIngestor::ingest_batch(
        std::vector<dispatcher::telemetry::TelemetryValue> values
    )
    {
        TelemetryIngestBatchResult batch_result;

        for (auto& value : values)
        {
            const auto result = ingest(std::move(value));
            batch_result.record(result.status());
        }

        return batch_result;
    }
}