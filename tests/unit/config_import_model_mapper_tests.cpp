#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_import_device.hpp>
#include <dispatcher/config/configuration_import_metadata.hpp>
#include <dispatcher/config/configuration_import_model.hpp>
#include <dispatcher/config/configuration_import_model_mapper.hpp>
#include <dispatcher/config/configuration_import_tag.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::config::ConfigurationImportDevice make_mapper_import_device()
    {
        return dispatcher::config::ConfigurationImportDevice{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .local_name = "device-1",
            .display_name = "Device 1",
            .enabled = true
        };
    }

    dispatcher::config::ConfigurationImportTag make_mapper_import_tag()
    {
        return dispatcher::config::ConfigurationImportTag{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .tag_id = "tag-temperature",
            .local_name = "temperature",
            .display_name = "Temperature",
            .data_type = "float64",
            .history_policy = "every_poll",
            .enabled = false
        };
    }

    dispatcher::config::ConfigurationImportModel make_mapper_import_model()
    {
        dispatcher::config::ConfigurationImportMetadata metadata;

        metadata.config_version = 7;
        metadata.status = "published";
        metadata.description = "Production configuration";
        metadata.source = "production.json";

        dispatcher::config::ConfigurationImportModel model(metadata);

        model.add_device(make_mapper_import_device());
        model.add_tag(make_mapper_import_tag());

        return model;
    }
}

TEST(ConfigurationSnapshotImportResultTests, SuccessResultContainsSnapshot)
{
    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        make_mapper_import_model()
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().config_version(), 7);

    EXPECT_EQ(
        result.snapshot().status(),
        dispatcher::domain::ConfigurationStatus::Published
    );
}

TEST(ConfigurationSnapshotImportResultTests, FailureResultDoesNotContainSnapshot)
{
    dispatcher::config::ConfigurationImportModel model =
        make_mapper_import_model();

    model.metadata().status = "archived";

    const auto result =
        dispatcher::config::ConfigurationImportModelMapper::map(model);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().operation, "configuration.import.map");
    EXPECT_EQ(result.error().resource, "production.json");
    EXPECT_EQ(result.error().field, "metadata.status");
    EXPECT_EQ(result.error().message, "configuration status is invalid");

    EXPECT_THROW(
        (void)result.snapshot(),
        std::logic_error
    );
}

TEST(ConfigurationImportModelMapperTests, MapsMetadata)
{
    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        make_mapper_import_model()
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().config_version(), 7);

    EXPECT_EQ(
        result.snapshot().status(),
        dispatcher::domain::ConfigurationStatus::Published
    );
}

TEST(ConfigurationImportModelMapperTests, MapsDevicesAndTags)
{
    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        make_mapper_import_model()
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    const auto& snapshot = result.snapshot();

    ASSERT_EQ(snapshot.device_count(), 1);
    ASSERT_EQ(snapshot.tag_count(), 1);

    ASSERT_EQ(snapshot.devices().size(), 1);
    ASSERT_EQ(snapshot.tags().size(), 1);

    const auto& device = snapshot.devices().front();

    EXPECT_EQ(device.organization_id(), dispatcher::domain::OrganizationId{ "org-1" });
    EXPECT_EQ(device.site_id(), dispatcher::domain::SiteId{ "site-1" });
    EXPECT_EQ(device.area_id(), dispatcher::domain::AreaId{ "area-1" });
    EXPECT_EQ(device.device_id(), dispatcher::domain::DeviceId{ "device-1" });
    EXPECT_EQ(device.local_name(), "device-1");
    EXPECT_EQ(device.display_name(), "Device 1");
    EXPECT_TRUE(device.enabled());

    const auto& tag = snapshot.tags().front();

    EXPECT_EQ(tag.organization_id(), dispatcher::domain::OrganizationId{ "org-1" });
    EXPECT_EQ(tag.site_id(), dispatcher::domain::SiteId{ "site-1" });
    EXPECT_EQ(tag.area_id(), dispatcher::domain::AreaId{ "area-1" });
    EXPECT_EQ(tag.device_id(), dispatcher::domain::DeviceId{ "device-1" });
    EXPECT_EQ(tag.tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(tag.local_name(), "temperature");
    EXPECT_EQ(tag.display_name(), "Temperature");

    EXPECT_EQ(
        tag.data_type(),
        dispatcher::domain::DataType::Float64
    );

    EXPECT_EQ(
        tag.history_policy(),
        dispatcher::domain::HistoryPolicy::EveryPoll
    );

    EXPECT_FALSE(tag.enabled());
}

TEST(ConfigurationImportModelMapperTests, MapsDraftStatus)
{
    auto model = make_mapper_import_model();

    model.metadata().status = "draft";

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    ASSERT_TRUE(result.ok());

    EXPECT_EQ(
        result.snapshot().status(),
        dispatcher::domain::ConfigurationStatus::Draft
    );
}

TEST(ConfigurationImportModelMapperTests, RejectsUnknownFormat)
{
    auto model = make_mapper_import_model();

    model.metadata().format = dispatcher::config::ConfigurationFormat::Unknown;

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_EQ(result.error().field, "metadata.format");
    EXPECT_EQ(
        result.error().message,
        "unsupported configuration import format"
    );
}

TEST(ConfigurationImportModelMapperTests, RejectsMissingConfigVersion)
{
    auto model = make_mapper_import_model();

    model.metadata().config_version = 0;

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "metadata.config_version");
    EXPECT_EQ(result.error().message, "configuration version is required");
}

TEST(ConfigurationImportModelMapperTests, RejectsInvalidStatus)
{
    auto model = make_mapper_import_model();

    model.metadata().status = "archived";

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "metadata.status");
    EXPECT_EQ(result.error().message, "configuration status is invalid");
}

TEST(ConfigurationImportModelMapperTests, RejectsIncompleteDeviceIdentity)
{
    auto model = make_mapper_import_model();

    dispatcher::config::ConfigurationImportModel invalid_model(
        model.metadata()
    );

    auto invalid_device = make_mapper_import_device();
    invalid_device.device_id.clear();

    invalid_model.add_device(invalid_device);

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        invalid_model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "devices[0]");
    EXPECT_EQ(result.error().message, "device identity is incomplete");
}

TEST(ConfigurationImportModelMapperTests, RejectsIncompleteTagIdentity)
{
    dispatcher::config::ConfigurationImportModel invalid_model =
        dispatcher::config::ConfigurationImportModel::create_empty();

    invalid_model.metadata().config_version = 7;
    invalid_model.metadata().status = "published";
    invalid_model.metadata().source = "production.json";

    invalid_model.add_device(make_mapper_import_device());

    auto invalid_tag = make_mapper_import_tag();
    invalid_tag.tag_id.clear();

    invalid_model.add_tag(invalid_tag);

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        invalid_model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "tags[0]");
    EXPECT_EQ(result.error().message, "tag identity is incomplete");
}

TEST(ConfigurationImportModelMapperTests, RejectsInvalidDataType)
{
    auto model = make_mapper_import_model();

    model.clear_tags();

    auto tag = make_mapper_import_tag();
    tag.data_type = "decimal";

    model.add_tag(tag);

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "tags[0].data_type");
    EXPECT_EQ(result.error().message, "tag data type is invalid");
}

TEST(ConfigurationImportModelMapperTests, RejectsInvalidHistoryPolicy)
{
    auto model = make_mapper_import_model();

    model.clear_tags();

    auto tag = make_mapper_import_tag();
    tag.history_policy = "archive_all";

    model.add_tag(tag);

    const auto result = dispatcher::config::ConfigurationImportModelMapper::map(
        model
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().field, "tags[0].history_policy");
    EXPECT_EQ(result.error().message, "tag history policy is invalid");
}