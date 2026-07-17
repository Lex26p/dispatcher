#include <dispatcher/storage/alarm_acknowledgement_storage.hpp>
#include <dispatcher/storage/alarm_event_storage.hpp>
#include <dispatcher/storage/configuration_storage.hpp>
#include <dispatcher/storage/history_storage.hpp>
#include <dispatcher/storage/storage_repository.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace
{
    class FakeHistoryStorage final : public dispatcher::storage::HistoryStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::history::HistorySample& sample
        ) override
        {
            (void)sample;
            append_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::history::HistorySample>& samples
        ) override
        {
            append_batch_size_ = samples.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::HistorySampleQueryResult query(
            const dispatcher::storage::HistoryStorageQuery& query
        ) const override
        {
            (void)query;
            query_called_ = true;

            return dispatcher::storage::HistorySampleQueryResult::success({});
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) override
        {
            last_removed_tag_id_ = tag_id.value();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] bool append_called() const noexcept
        {
            return append_called_;
        }

        [[nodiscard]] std::size_t append_batch_size() const noexcept
        {
            return append_batch_size_;
        }

        [[nodiscard]] bool query_called() const noexcept
        {
            return query_called_;
        }

        [[nodiscard]] const std::string& last_removed_tag_id() const noexcept
        {
            return last_removed_tag_id_;
        }

        [[nodiscard]] bool clear_called() const noexcept
        {
            return clear_called_;
        }

    private:
        bool append_called_{ false };
        std::size_t append_batch_size_{ 0 };
        mutable bool query_called_{ false };
        std::string last_removed_tag_id_;
        bool clear_called_{ false };
    };

    class FakeAlarmEventStorage final
        : public dispatcher::storage::AlarmEventStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::alarm::AlarmRuntimeEvent& event
        ) override
        {
            (void)event;
            append_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmRuntimeEvent>& events
        ) override
        {
            append_batch_size_ = events.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::AlarmEventQueryResult query(
            const dispatcher::storage::AlarmEventStorageQuery& query
        ) const override
        {
            (void)query;
            query_called_ = true;

            return dispatcher::storage::AlarmEventQueryResult::success({});
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            last_removed_alarm_id_ = alarm_id.value();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) override
        {
            last_removed_tag_id_ = tag_id.value();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] bool query_called() const noexcept
        {
            return query_called_;
        }

        [[nodiscard]] const std::string& last_removed_alarm_id() const noexcept
        {
            return last_removed_alarm_id_;
        }

    private:
        bool append_called_{ false };
        std::size_t append_batch_size_{ 0 };
        mutable bool query_called_{ false };
        std::string last_removed_alarm_id_;
        std::string last_removed_tag_id_;
        bool clear_called_{ false };
    };

    class FakeAlarmAcknowledgementStorage final
        : public dispatcher::storage::AlarmAcknowledgementStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::alarm::AlarmAcknowledgementRecord& record
        ) override
        {
            (void)record;
            append_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
            records
        ) override
        {
            append_batch_size_ = records.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::AlarmAcknowledgementQueryResult query(
            const dispatcher::storage::AlarmAcknowledgementStorageQuery& query
        ) const override
        {
            (void)query;
            query_called_ = true;

            return dispatcher::storage::AlarmAcknowledgementQueryResult::success(
                {}
            );
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            last_removed_alarm_id_ = alarm_id.value();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_operator(
            const std::string& operator_id
        ) override
        {
            last_removed_operator_id_ = operator_id;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] bool query_called() const noexcept
        {
            return query_called_;
        }

        [[nodiscard]] const std::string& last_removed_operator_id() const noexcept
        {
            return last_removed_operator_id_;
        }

    private:
        bool append_called_{ false };
        std::size_t append_batch_size_{ 0 };
        mutable bool query_called_{ false };
        std::string last_removed_alarm_id_;
        std::string last_removed_operator_id_;
        bool clear_called_{ false };
    };

    class FakeConfigurationStorage final
        : public dispatcher::storage::ConfigurationStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult save(
            const dispatcher::domain::ConfigurationSnapshot& snapshot
        ) override
        {
            last_saved_config_version_ = snapshot.config_version();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::ConfigurationSnapshotQueryResult query(
            const dispatcher::storage::ConfigurationStorageQuery& query
        ) const override
        {
            (void)query;
            query_called_ = true;

            return dispatcher::storage::ConfigurationSnapshotQueryResult::success(
                {}
            );
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_version(
            std::uint64_t config_version
        ) override
        {
            last_removed_config_version_ = config_version;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] bool query_called() const noexcept
        {
            return query_called_;
        }

        [[nodiscard]] std::uint64_t last_removed_config_version() const noexcept
        {
            return last_removed_config_version_;
        }

    private:
        std::uint64_t last_saved_config_version_{ 0 };
        mutable bool query_called_{ false };
        std::uint64_t last_removed_config_version_{ 0 };
        bool clear_called_{ false };
    };

    class FakeStorageRepository final
        : public dispatcher::storage::StorageRepository
    {
    public:
        [[nodiscard]] dispatcher::storage::HistoryStorage& history_storage()
            noexcept override
        {
            return history_storage_;
        }

        [[nodiscard]] const dispatcher::storage::HistoryStorage&
            history_storage() const noexcept override
        {
            return history_storage_;
        }

        [[nodiscard]] dispatcher::storage::AlarmEventStorage&
            alarm_event_storage() noexcept override
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] const dispatcher::storage::AlarmEventStorage&
            alarm_event_storage() const noexcept override
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] dispatcher::storage::AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() noexcept override
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] const dispatcher::storage::AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() const noexcept override
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] dispatcher::storage::ConfigurationStorage&
            configuration_storage() noexcept override
        {
            return configuration_storage_;
        }

        [[nodiscard]] const dispatcher::storage::ConfigurationStorage&
            configuration_storage() const noexcept override
        {
            return configuration_storage_;
        }

        [[nodiscard]] FakeHistoryStorage& fake_history_storage() noexcept
        {
            return history_storage_;
        }

        [[nodiscard]] FakeAlarmEventStorage& fake_alarm_event_storage() noexcept
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] FakeAlarmAcknowledgementStorage&
            fake_alarm_acknowledgement_storage() noexcept
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] FakeConfigurationStorage& fake_configuration_storage()
            noexcept
        {
            return configuration_storage_;
        }

    private:
        FakeHistoryStorage history_storage_;
        FakeAlarmEventStorage alarm_event_storage_;
        FakeAlarmAcknowledgementStorage alarm_acknowledgement_storage_;
        FakeConfigurationStorage configuration_storage_;
    };
}

TEST(StorageRepositoryTests, RepositoryExposesMutableStorageInterfaces)
{
    FakeStorageRepository repository;

    const auto history_query_result =
        repository.history_storage().query(
            dispatcher::storage::HistoryStorageQuery{}
        );

    EXPECT_TRUE(history_query_result.ok());
    EXPECT_TRUE(repository.fake_history_storage().query_called());

    const auto alarm_event_query_result =
        repository.alarm_event_storage().query(
            dispatcher::storage::AlarmEventStorageQuery{}
        );

    EXPECT_TRUE(alarm_event_query_result.ok());
    EXPECT_TRUE(repository.fake_alarm_event_storage().query_called());

    const auto acknowledgement_query_result =
        repository.alarm_acknowledgement_storage().query(
            dispatcher::storage::AlarmAcknowledgementStorageQuery{}
        );

    EXPECT_TRUE(acknowledgement_query_result.ok());
    EXPECT_TRUE(
        repository.fake_alarm_acknowledgement_storage().query_called()
    );

    const auto configuration_query_result =
        repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{}
        );

    EXPECT_TRUE(configuration_query_result.ok());
    EXPECT_TRUE(repository.fake_configuration_storage().query_called());
}

TEST(StorageRepositoryTests, RepositoryExposesConstStorageInterfaces)
{
    FakeStorageRepository repository;

    const auto& const_repository = repository;

    const auto history_query_result =
        const_repository.history_storage().query(
            dispatcher::storage::HistoryStorageQuery{}
        );

    EXPECT_TRUE(history_query_result.ok());

    const auto alarm_event_query_result =
        const_repository.alarm_event_storage().query(
            dispatcher::storage::AlarmEventStorageQuery{}
        );

    EXPECT_TRUE(alarm_event_query_result.ok());

    const auto acknowledgement_query_result =
        const_repository.alarm_acknowledgement_storage().query(
            dispatcher::storage::AlarmAcknowledgementStorageQuery{}
        );

    EXPECT_TRUE(acknowledgement_query_result.ok());

    const auto configuration_query_result =
        const_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{}
        );

    EXPECT_TRUE(configuration_query_result.ok());
}

TEST(StorageRepositoryTests, RepositoryRoutesOperationsToConcreteStorages)
{
    FakeStorageRepository repository;

    const auto remove_history_result =
        repository.history_storage().remove_by_tag(
            dispatcher::domain::TagId{ "tag-1" }
        );

    EXPECT_TRUE(remove_history_result.ok());
    EXPECT_EQ(
        repository.fake_history_storage().last_removed_tag_id(),
        "tag-1"
    );

    const auto remove_alarm_event_result =
        repository.alarm_event_storage().remove_by_alarm(
            dispatcher::domain::AlarmId{ "alarm-1" }
        );

    EXPECT_TRUE(remove_alarm_event_result.ok());
    EXPECT_EQ(
        repository.fake_alarm_event_storage().last_removed_alarm_id(),
        "alarm-1"
    );

    const auto remove_acknowledgement_result =
        repository.alarm_acknowledgement_storage().remove_by_operator(
            "operator-1"
        );

    EXPECT_TRUE(remove_acknowledgement_result.ok());
    EXPECT_EQ(
        repository.fake_alarm_acknowledgement_storage()
        .last_removed_operator_id(),
        "operator-1"
    );

    const auto remove_configuration_result =
        repository.configuration_storage().remove_by_version(7);

    EXPECT_TRUE(remove_configuration_result.ok());
    EXPECT_EQ(
        repository.fake_configuration_storage().last_removed_config_version(),
        7
    );
}