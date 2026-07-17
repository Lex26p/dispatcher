#include <dispatcher/alarm/alarm_runtime.hpp>

#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_validation.hpp>
#include <dispatcher/domain/configuration_status.hpp>

#include <utility>

namespace
{
    dispatcher::alarm::AlarmConfigurationSnapshot make_default_alarm_configuration()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .build();
    }
}

namespace dispatcher::alarm
{
    AlarmRuntime::AlarmRuntime()
        : evaluator_(state_store_)
        , configuration_snapshot_(make_default_alarm_configuration())
    {
        rebuild_configuration_indexes();
    }

    const AlarmConfigurationSnapshot& AlarmRuntime::configuration_snapshot()
        const noexcept
    {
        return configuration_snapshot_;
    }

    dispatcher::common::ValidationResult AlarmRuntime::reload_configuration(
        AlarmConfigurationSnapshot snapshot
    )
    {
        dispatcher::common::ValidationResult result;

        if (!snapshot.published())
        {
            result.add_error(
                "status",
                "alarm configuration snapshot must be published"
            );
        }

        const auto validation_result = validate_alarm_configuration_snapshot(
            snapshot
        );

        for (const auto& error : validation_result.errors())
        {
            result.add_error(
                error.field,
                error.message
            );
        }

        if (result.has_errors())
        {
            return result;
        }

        configuration_snapshot_ = std::move(snapshot);
        rebuild_configuration_indexes();
        prune_state_store_to_configuration();

        return result;
    }

    AlarmAcknowledgementResult AlarmRuntime::acknowledge(
        const dispatcher::domain::AlarmId& alarm_id
    )
    {
        const auto alarm_definition =
            configuration_snapshot_.find_by_alarm_id(alarm_id);

        if (!alarm_definition.has_value())
        {
            return AlarmAcknowledgementResult(
                AlarmAcknowledgementStatus::UnknownAlarm,
                AlarmState::Normal,
                AlarmState::Normal,
                std::nullopt
            );
        }

        const auto previous_state = state_store_.state_of(alarm_id);

        if (previous_state == AlarmState::Acknowledged)
        {
            return AlarmAcknowledgementResult(
                AlarmAcknowledgementStatus::AlreadyAcknowledged,
                previous_state,
                previous_state,
                std::nullopt
            );
        }

        if (previous_state != AlarmState::Active)
        {
            return AlarmAcknowledgementResult(
                AlarmAcknowledgementStatus::NotActive,
                previous_state,
                previous_state,
                std::nullopt
            );
        }

        const auto event_timestamp = AlarmRuntimeEvent::Clock::now();

        auto event = AlarmRuntimeEvent(
            alarm_definition->alarm_id(),
            alarm_definition->tag_id(),
            alarm_definition->severity(),
            AlarmTransitionType::Acknowledged,
            previous_state,
            AlarmState::Acknowledged,
            event_timestamp,
            event_timestamp,
            ++acknowledgement_sequence_
        );

        state_store_.set_state(
            alarm_definition->alarm_id(),
            AlarmState::Acknowledged
        );

        event_store_.append(event);

        const auto statistics_result = AlarmEvaluationResult(
            AlarmEvaluationStatus::Evaluated,
            AlarmTransitionType::Acknowledged,
            previous_state,
            AlarmState::Acknowledged,
            true,
            event
        );

        statistics_.record(
            statistics_result,
            true
        );

        return AlarmAcknowledgementResult(
            AlarmAcknowledgementStatus::Acknowledged,
            previous_state,
            AlarmState::Acknowledged,
            std::move(event)
        );
    }

    AlarmAcknowledgementResult AlarmRuntime::acknowledge(
        const AlarmAcknowledgementCommand& command
    )
    {
        const auto validation_result =
            validate_alarm_acknowledgement_command(command);

        if (validation_result.has_errors())
        {
            const auto result = AlarmAcknowledgementResult(
                AlarmAcknowledgementStatus::InvalidCommand,
                AlarmState::Normal,
                AlarmState::Normal,
                std::nullopt
            );

            acknowledgement_store_.append(
                AlarmAcknowledgementRecord(
                    command.alarm_id(),
                    command.operator_id(),
                    command.comment(),
                    result.status(),
                    result.previous_state(),
                    result.new_state(),
                    AlarmAcknowledgementRecord::Clock::now(),
                    std::nullopt
                )
            );

            return result;
        }

        const auto result = acknowledge(command.alarm_id());

        std::optional<std::uint64_t> event_sequence;

        if (result.event().has_value())
        {
            event_sequence = result.event()->sequence();
        }

        acknowledgement_store_.append(
            AlarmAcknowledgementRecord(
                command.alarm_id(),
                command.operator_id(),
                command.comment(),
                result.status(),
                result.previous_state(),
                result.new_state(),
                AlarmAcknowledgementRecord::Clock::now(),
                event_sequence
            )
        );

        return result;
    }

    AlarmEvaluationResult AlarmRuntime::evaluate(
        const AlarmDefinition& alarm_definition,
        const ThresholdAlarmCondition& condition,
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    )
    {
        auto evaluation_result = evaluator_.evaluate(
            alarm_definition,
            condition,
            telemetry_value
        );

        const auto event_stored = event_store_.append_from_evaluation_result(
            evaluation_result
        );

        statistics_.record(
            evaluation_result,
            event_stored
        );

        return evaluation_result;
    }

    AlarmEvaluationBatchResult AlarmRuntime::evaluate_batch(
        const std::vector<AlarmEvaluationCandidate>& candidates
    )
    {
        AlarmEvaluationBatchResult batch_result;

        for (const auto& candidate : candidates)
        {
            auto result = evaluate(
                candidate.alarm_definition,
                candidate.condition,
                candidate.telemetry_value
            );

            batch_result.record(std::move(result));
        }

        return batch_result;
    }

    AlarmConfiguredEvaluationResult AlarmRuntime::evaluate_configured(
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    )
    {
        AlarmConfiguredEvaluationResult configured_result;

        [[maybe_unused]] const auto expired_suppression_removed_count =
            suppression_store_.remove_expired(
                AlarmSuppressionStore::Clock::now()
            );

        const auto alarm_definitions_iterator =
            alarm_definitions_by_tag_id_.find(
                telemetry_value.tag_id().value()
            );

        if (alarm_definitions_iterator == alarm_definitions_by_tag_id_.end())
        {
            return configured_result;
        }

        for (const auto& alarm_definition : alarm_definitions_iterator->second)
        {
            configured_result.record_configured_alarm();

            if (suppression_store_.is_active(alarm_definition.alarm_id()))
            {
                configured_result.record_suppressed_alarm();
                continue;
            }

            const auto condition_definition_iterator =
                condition_definitions_by_alarm_id_.find(
                    alarm_definition.alarm_id().value()
                );

            if (condition_definition_iterator
                == condition_definitions_by_alarm_id_.end())
            {
                configured_result.record_missing_condition();
                continue;
            }

            auto evaluation_result = evaluate(
                alarm_definition,
                condition_definition_iterator->second.condition(),
                telemetry_value
            );

            configured_result.batch_result().record(
                std::move(evaluation_result)
            );
        }

        return configured_result;
    }

    AlarmConfiguredEvaluationResult AlarmRuntime::evaluate_configured_batch(
        const std::vector<dispatcher::telemetry::TelemetryValue>& telemetry_values
    )
    {
        AlarmConfiguredEvaluationResult configured_result;

        for (const auto& telemetry_value : telemetry_values)
        {
            auto item_result = evaluate_configured(telemetry_value);

            for (auto result : item_result.batch_result().results())
            {
                configured_result.batch_result().record(std::move(result));
            }

            for (std::uint64_t index = 0;
                index < item_result.configured_alarm_count();
                ++index)
            {
                configured_result.record_configured_alarm();
            }

            for (std::uint64_t index = 0;
                index < item_result.missing_condition_count();
                ++index)
            {
                configured_result.record_missing_condition();
            }

            for (std::uint64_t index = 0;
                index < item_result.suppressed_alarm_count();
                ++index)
            {
                configured_result.record_suppressed_alarm();
            }
        }

        return configured_result;
    }

    const AlarmStateStore& AlarmRuntime::state_store() const noexcept
    {
        return state_store_;
    }

    const AlarmEventStore& AlarmRuntime::event_store() const noexcept
    {
        return event_store_;
    }

    const AlarmAcknowledgementStore& AlarmRuntime::acknowledgement_store()
        const noexcept
    {
        return acknowledgement_store_;
    }

    AlarmSuppressionStore& AlarmRuntime::suppression_store() noexcept
    {
        return suppression_store_;
    }

    const AlarmSuppressionStore& AlarmRuntime::suppression_store()
        const noexcept
    {
        return suppression_store_;
    }

    AlarmSuppressionRuntimeSnapshot
        AlarmRuntime::runtime_suppression_snapshot() const noexcept
    {
        return suppression_store_.runtime_snapshot();
    }

    AlarmSuppressionResult AlarmRuntime::suppress(
        const AlarmSuppressionCommand& command
    )
    {
        return suppression_store_.apply(command);
    }

    AlarmSuppressionResult AlarmRuntime::clear_suppression(
        const dispatcher::domain::AlarmId& alarm_id
    )
    {
        return suppression_store_.clear(alarm_id);
    }

    bool AlarmRuntime::is_suppressed(
        const dispatcher::domain::AlarmId& alarm_id
    ) const noexcept
    {
        return suppression_store_.is_active(alarm_id);
    }

    std::vector<AlarmDefinition> AlarmRuntime::alarms_by_state(
        AlarmState state
    ) const
    {
        std::vector<AlarmDefinition> result;

        for (const auto& alarm_definition :
            configuration_snapshot_.alarm_catalog().definitions())
        {
            if (state_store_.state_of(alarm_definition.alarm_id()) == state)
            {
                result.push_back(alarm_definition);
            }
        }

        return result;
    }

    std::vector<AlarmDefinition> AlarmRuntime::active_alarms() const
    {
        return alarms_by_state(AlarmState::Active);
    }

    std::vector<AlarmDefinition> AlarmRuntime::acknowledged_alarms() const
    {
        return alarms_by_state(AlarmState::Acknowledged);
    }

    std::vector<AlarmDefinition> AlarmRuntime::normal_alarms() const
    {
        return alarms_by_state(AlarmState::Normal);
    }

    std::vector<AlarmDefinition> AlarmRuntime::unacknowledged_alarms() const
    {
        return active_alarms();
    }

    AlarmOperatorSnapshot AlarmRuntime::operator_snapshot() const
    {
        AlarmOperatorSnapshot snapshot;

        snapshot.configuration_version =
            configuration_snapshot_.config_version();

        snapshot.configured_alarm_count =
            configuration_snapshot_.alarm_catalog().size();

        snapshot.event_store_size = event_store_.size();
        snapshot.acknowledgement_store_size = acknowledgement_store_.size();

        snapshot.activated_count = statistics_.activated_count();
        snapshot.acknowledged_count = statistics_.acknowledged_count();
        snapshot.cleared_count = statistics_.cleared_count();

        const auto suppression_snapshot =
            suppression_store_.runtime_snapshot();

        snapshot.suppression_store_size = suppression_snapshot.store_size;
        snapshot.shelved_alarm_count = suppression_snapshot.shelved_count;
        snapshot.suppressed_alarm_count =
            suppression_snapshot.suppressed_count;
        snapshot.inhibited_alarm_count = suppression_snapshot.inhibited_count;
        snapshot.operator_controlled_suppression_count =
            suppression_snapshot.operator_controlled_count;
        snapshot.system_controlled_suppression_count =
            suppression_snapshot.system_controlled_count;

        for (const auto& alarm_definition :
            configuration_snapshot_.alarm_catalog().definitions())
        {
            switch (state_store_.state_of(alarm_definition.alarm_id()))
            {
            case AlarmState::Normal:
                ++snapshot.normal_alarm_count;
                break;

            case AlarmState::Active:
                ++snapshot.active_alarm_count;
                ++snapshot.unacknowledged_alarm_count;
                break;

            case AlarmState::Acknowledged:
                ++snapshot.acknowledged_alarm_count;
                break;
            }
        }

        return snapshot;
    }

    const AlarmRuntimeStatistics& AlarmRuntime::statistics() const noexcept
    {
        return statistics_;
    }

    AlarmRuntimeSnapshot AlarmRuntime::runtime_snapshot() const noexcept
    {
        return AlarmRuntimeSnapshot{
            .state_store_size = state_store_.size(),
            .event_store_size = event_store_.size(),
            .acknowledgement_store_size = acknowledgement_store_.size(),

            .total_count = statistics_.total_count(),
            .evaluated_count = statistics_.evaluated_count(),
            .skipped_count = statistics_.skipped_count(),

            .disabled_alarm_count = statistics_.disabled_alarm_count(),
            .tag_mismatch_count = statistics_.tag_mismatch_count(),
            .unsupported_value_type_count =
                statistics_.unsupported_value_type_count(),

            .activated_count = statistics_.activated_count(),
            .acknowledged_count = statistics_.acknowledged_count(),
            .cleared_count = statistics_.cleared_count(),
            .no_transition_count = statistics_.no_transition_count(),
            .stored_event_count = statistics_.stored_event_count(),

            .configuration_version = configuration_snapshot_.config_version(),
            .configured_alarm_count =
                configuration_snapshot_.alarm_catalog().size(),
            .indexed_tag_count = alarm_definitions_by_tag_id_.size(),
            .indexed_condition_count =
                condition_definitions_by_alarm_id_.size(),

            .suppression = suppression_store_.runtime_snapshot()
        };
    }

    void AlarmRuntime::reset_statistics() noexcept
    {
        statistics_.reset();
    }

    void AlarmRuntime::rebuild_configuration_indexes()
    {
        alarm_definitions_by_tag_id_.clear();
        condition_definitions_by_alarm_id_.clear();

        for (const auto& alarm_definition :
            configuration_snapshot_.alarm_catalog().definitions())
        {
            alarm_definitions_by_tag_id_[alarm_definition.tag_id().value()]
                .push_back(alarm_definition);
        }

        for (const auto& condition_definition :
            configuration_snapshot_.condition_catalog().definitions())
        {
            condition_definitions_by_alarm_id_.emplace(
                condition_definition.alarm_id().value(),
                condition_definition
            );
        }
    }

    void AlarmRuntime::prune_state_store_to_configuration()
    {
        std::vector<dispatcher::domain::AlarmId> configured_alarm_ids;

        configured_alarm_ids.reserve(
            configuration_snapshot_.alarm_catalog().size()
        );

        for (const auto& alarm_definition :
            configuration_snapshot_.alarm_catalog().definitions())
        {
            configured_alarm_ids.push_back(
                alarm_definition.alarm_id()
            );
        }

        state_store_.retain_only(configured_alarm_ids);
    }
}