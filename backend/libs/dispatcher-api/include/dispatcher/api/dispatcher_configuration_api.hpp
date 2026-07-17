#pragma once

#include <dispatcher/api/configuration_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <string>

namespace dispatcher::api
{
    class DispatcherConfigurationApi final : public ConfigurationApi
    {
    public:
        explicit DispatcherConfigurationApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] ConfigurationQueryApiResult query(
            const ConfigurationQueryRequest& request
        ) const override;

        [[nodiscard]] ConfigurationReloadApiResult reload(
            const ConfigurationReloadRequest& request
        ) override;

        [[nodiscard]] ConfigurationExportApiResult export_current(
            dispatcher::config::ConfigurationExportOptions options = {},
            std::string name = {}
        ) const override;

        [[nodiscard]] ConfigurationReloadApiResult import_document(
            const dispatcher::config::ConfigurationDocument& document
        ) override;

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };
    };
}