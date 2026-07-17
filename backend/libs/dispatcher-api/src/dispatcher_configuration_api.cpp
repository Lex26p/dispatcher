#include <dispatcher/api/dispatcher_configuration_api.hpp>

#include <dispatcher/api/api_page.hpp>
#include <dispatcher/api/api_status_mapping.hpp>
#include <dispatcher/api/api_status_mapping.hpp>
#include <dispatcher/config/configuration_exporter.hpp>
#include <dispatcher/config/configuration_importer.hpp>

#include <utility>
#include <string>

namespace dispatcher::api
{
    DispatcherConfigurationApi::DispatcherConfigurationApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
    {
    }

    ConfigurationQueryApiResult DispatcherConfigurationApi::query(
        const ConfigurationQueryRequest& request
    ) const
    {
        if (runtime_ == nullptr)
        {
            return ConfigurationQueryApiResult::failure(
                ApiStatus::InternalError,
                "configuration.query",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        auto* storage_repository = runtime_->storage_repository();

        if (storage_repository == nullptr)
        {
            return ConfigurationQueryApiResult::failure(
                ApiStatus::StorageError,
                "configuration.query",
                "storage_repository",
                {},
                "runtime does not have a storage repository"
            );
        }

        const auto storage_query_result =
            storage_repository->configuration_storage().query(
                request.to_storage_query()
            );

        if (storage_query_result.failed())
        {
            return ConfigurationQueryApiResult::failure(
                map_storage_status_to_api_status(
                    storage_query_result.status()
                ),
                storage_query_result.error().has_operation()
                ? storage_query_result.error().operation
                : "configuration.query",
                storage_query_result.error().key,
                {},
                storage_query_result.error().message
            );
        }

        auto snapshots = storage_query_result.snapshots();

        const auto page = ApiPage::from_request(
            request.page,
            snapshots.size(),
            snapshots.size()
        );

        return ConfigurationQueryApiResult::success(
            std::move(snapshots),
            page
        );
    }

    ConfigurationReloadApiResult DispatcherConfigurationApi::reload(
        const ConfigurationReloadRequest& request
    )
    {
        if (runtime_ == nullptr)
        {
            return ConfigurationReloadApiResult::failure(
                ApiStatus::InternalError,
                "configuration.reload",
                std::to_string(request.config_version()),
                {},
                "runtime instance is not available"
            );
        }

        const auto reload_result =
            runtime_->reload_telemetry_configuration(request.snapshot());

        if (reload_result.has_errors())
        {
            const auto& first_error = reload_result.errors().front();

            return ConfigurationReloadApiResult::failure(
                ApiStatus::ValidationError,
                "configuration.reload",
                std::to_string(request.config_version()),
                first_error.field,
                first_error.message
            );
        }

        return ConfigurationReloadApiResult::success(
            runtime_->telemetry_ingestor().configuration_snapshot()
        );
    }

    dispatcher::runtime::DispatcherRuntime&
        DispatcherConfigurationApi::runtime() noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime&
        DispatcherConfigurationApi::runtime() const noexcept
    {
        return *runtime_;
    }

    ConfigurationExportApiResult DispatcherConfigurationApi::export_current(
        dispatcher::config::ConfigurationExportOptions options,
        std::string name
    ) const
    {
        const auto export_result =
            dispatcher::config::ConfigurationExporter::export_snapshot(
                runtime_->telemetry_ingestor().configuration_snapshot(),
                std::move(options),
                std::move(name)
            );

        if (export_result.failed())
        {
            return ConfigurationExportApiResult::failure(
                map_configuration_io_status_to_api_status(
                    export_result.status()
                ),
                export_result.error().operation,
                export_result.error().resource,
                export_result.error().field,
                export_result.error().message
            );
        }

        return ConfigurationExportApiResult::success(
            export_result.document()
        );
    }

    ConfigurationReloadApiResult DispatcherConfigurationApi::import_document(
        const dispatcher::config::ConfigurationDocument& document
    )
    {
        const auto import_result =
            dispatcher::config::ConfigurationImporter::import_document(
                document
            );

        if (import_result.failed())
        {
            return ConfigurationReloadApiResult::failure(
                map_configuration_io_status_to_api_status(
                    import_result.status()
                ),
                import_result.error().operation,
                import_result.error().resource,
                import_result.error().field,
                import_result.error().message
            );
        }

        const auto validation = runtime_->reload_telemetry_configuration(
            import_result.snapshot()
        );

        if (validation.has_errors())
        {
            const auto& error = validation.errors().front();

            return ConfigurationReloadApiResult::failure(
                ApiStatus::ValidationError,
                "configuration.import.reload",
                document.name(),
                error.field,
                error.message
            );
        }

        return ConfigurationReloadApiResult::success(
            runtime_->telemetry_ingestor().configuration_snapshot()
        );
    }
}