#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_command_validation.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_store.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/alarm/alarm_configured_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/alarm/alarm_evaluation_batch_result.hpp>
#include <dispatcher/alarm/alarm_evaluation_candidate.hpp>
#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_evaluator.hpp>
#include <dispatcher/alarm/alarm_event_store.hpp>
#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime_statistics.hpp>
#include <dispatcher/alarm/alarm_state_store.hpp>
#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_result.hpp>
#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>
#include <dispatcher/alarm/alarm_suppression_store.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmRuntime
    {
    public:
        AlarmRuntime();

        [[nodiscard]] const AlarmConfigurationSnapshot& configuration_snapshot()
            const noexcept;

        [[nodiscard]] dispatcher::common::ValidationResult reload_configuration(
            AlarmConfigurationSnapshot snapshot
        );

        [[nodiscard]] AlarmAcknowledgementResult acknowledge(
            const dispatcher::domain::AlarmId& alarm_id
        );

        [[nodiscard]] AlarmAcknowledgementResult acknowledge(
            const AlarmAcknowledgementCommand& command
        );

        [[nodiscard]] AlarmEvaluationResult evaluate(
            const AlarmDefinition& alarm_definition,
            const ThresholdAlarmCondition& condition,
            const dispatcher::telemetry::TelemetryValue& telemetry_value
        );

        [[nodiscard]] AlarmEvaluationBatchResult evaluate_batch(
            const std::vector<AlarmEvaluationCandidate>& candidates
        );

        [[nodiscard]] AlarmConfiguredEvaluationResult evaluate_configured(
            const dispatcher::telemetry::TelemetryValue& telemetry_value
        );

        [[nodiscard]] AlarmConfiguredEvaluationResult evaluate_configured_batch(
            const std::vector<dispatcher::telemetry::TelemetryValue>& telemetry_values
        );

        [[nodiscard]] const AlarmStateStore& state_store() const noexcept;

        [[nodiscard]] const AlarmEventStore& event_store() const noexcept;

        [[nodiscard]] const AlarmAcknowledgementStore& acknowledgement_store()
            const noexcept;

        [[nodiscard]] AlarmSuppressionStore& suppression_store() noexcept;

        [[nodiscard]] const AlarmSuppressionStore& suppression_store()
            const noexcept;

        [[nodiscard]] AlarmSuppressionRuntimeSnapshot
            runtime_suppression_snapshot() const noexcept;

        [[nodiscard]] AlarmSuppressionResult suppress(
            const AlarmSuppressionCommand& command
        );

        [[nodiscard]] AlarmSuppressionResult clear_suppression(
            const dispatcher::domain::AlarmId& alarm_id
        );

        [[nodiscard]] bool is_suppressed(
            const dispatcher::domain::AlarmId& alarm_id
        ) const noexcept;

        [[nodiscard]] std::vector<AlarmDefinition> alarms_by_state(
            AlarmState state
        ) const;

        [[nodiscard]] std::vector<AlarmDefinition> active_alarms() const;

        [[nodiscard]] std::vector<AlarmDefinition> acknowledged_alarms() const;

        [[nodiscard]] std::vector<AlarmDefinition> normal_alarms() const;

        [[nodiscard]] std::vector<AlarmDefinition> unacknowledged_alarms() const;

        [[nodiscard]] AlarmOperatorSnapshot operator_snapshot() const;

        [[nodiscard]] const AlarmRuntimeStatistics& statistics() const noexcept;

        [[nodiscard]] AlarmRuntimeSnapshot runtime_snapshot() const noexcept;

        void reset_statistics() noexcept;

    private:
        void rebuild_configuration_indexes();

        void prune_state_store_to_configuration();

        AlarmStateStore state_store_;
        AlarmEventStore event_store_;
        AlarmAcknowledgementStore acknowledgement_store_;
        AlarmSuppressionStore suppression_store_;
        AlarmEvaluator evaluator_;
        AlarmRuntimeStatistics statistics_;
        AlarmConfigurationSnapshot configuration_snapshot_;
        std::uint64_t acknowledgement_sequence_{ 0 };

        std::unordered_map<std::string, std::vector<AlarmDefinition>>
            alarm_definitions_by_tag_id_;

        std::unordered_map<std::string, AlarmConditionDefinition>
            condition_definitions_by_alarm_id_;
    };
}