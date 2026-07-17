#include <dispatcher/alarm/alarm_evaluation_batch_result.hpp>
#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_runtime_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime_statistics.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>

#include <gtest/gtest.h>

#include <optional>

TEST(AlarmAcknowledgementModelTests, TransitionTypeHasAcknowledgedValue)
{
    EXPECT_EQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmTransitionType::Acknowledged
        ),
        "acknowledged"
    );
}

TEST(AlarmAcknowledgementModelTests, EvaluationResultRecognizesAcknowledgedTransition)
{
    const dispatcher::alarm::AlarmEvaluationResult result(
        dispatcher::alarm::AlarmEvaluationStatus::Evaluated,
        dispatcher::alarm::AlarmTransitionType::Acknowledged,
        dispatcher::alarm::AlarmState::Active,
        dispatcher::alarm::AlarmState::Acknowledged,
        true,
        std::nullopt
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.transitioned());

    EXPECT_FALSE(result.activated());
    EXPECT_TRUE(result.acknowledged());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(
        result.previous_state(),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(
        result.new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );
}

TEST(AlarmAcknowledgementModelTests, RuntimeStatisticsCountsAcknowledgedTransition)
{
    dispatcher::alarm::AlarmRuntimeStatistics statistics;

    const dispatcher::alarm::AlarmEvaluationResult result(
        dispatcher::alarm::AlarmEvaluationStatus::Evaluated,
        dispatcher::alarm::AlarmTransitionType::Acknowledged,
        dispatcher::alarm::AlarmState::Active,
        dispatcher::alarm::AlarmState::Acknowledged,
        true,
        std::nullopt
    );

    statistics.record(result, true);

    EXPECT_EQ(statistics.total_count(), 1);
    EXPECT_EQ(statistics.evaluated_count(), 1);
    EXPECT_EQ(statistics.activated_count(), 0);
    EXPECT_EQ(statistics.acknowledged_count(), 1);
    EXPECT_EQ(statistics.cleared_count(), 0);
    EXPECT_EQ(statistics.no_transition_count(), 0);
    EXPECT_EQ(statistics.stored_event_count(), 1);
}

TEST(AlarmAcknowledgementModelTests, BatchResultCountsAcknowledgedTransition)
{
    dispatcher::alarm::AlarmEvaluationBatchResult batch_result;

    batch_result.record(
        dispatcher::alarm::AlarmEvaluationResult(
            dispatcher::alarm::AlarmEvaluationStatus::Evaluated,
            dispatcher::alarm::AlarmTransitionType::Acknowledged,
            dispatcher::alarm::AlarmState::Active,
            dispatcher::alarm::AlarmState::Acknowledged,
            true,
            std::nullopt
        )
    );

    EXPECT_EQ(batch_result.total_count(), 1);
    EXPECT_EQ(batch_result.evaluated_count(), 1);
    EXPECT_EQ(batch_result.activated_count(), 0);
    EXPECT_EQ(batch_result.acknowledged_count(), 1);
    EXPECT_EQ(batch_result.cleared_count(), 0);
    EXPECT_EQ(batch_result.no_transition_count(), 0);
}

TEST(AlarmAcknowledgementModelTests, RuntimeSnapshotExposesAcknowledgedCount)
{
    dispatcher::alarm::AlarmRuntimeSnapshot snapshot;

    snapshot.acknowledged_count = 3;

    EXPECT_EQ(snapshot.acknowledged_count, 3);
}