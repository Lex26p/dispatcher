#pragma once

#include <dispatcher/api/configuration_export_api_result.hpp>
#include <dispatcher/api/configuration_query_api_result.hpp>
#include <dispatcher/api/configuration_query_request.hpp>
#include <dispatcher/api/configuration_reload_api_result.hpp>
#include <dispatcher/api/configuration_reload_request.hpp>
#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_export_options.hpp>

#include <string>

namespace dispatcher::api
{
    class ConfigurationApi
    {
    public:
        virtual ~ConfigurationApi() = default;

        [[nodiscard]] virtual ConfigurationQueryApiResult query(
            const ConfigurationQueryRequest& request
        ) const = 0;

        [[nodiscard]] virtual ConfigurationReloadApiResult reload(
            const ConfigurationReloadRequest& request
        ) = 0;

        [[nodiscard]] virtual ConfigurationExportApiResult export_current(
            dispatcher::config::ConfigurationExportOptions options = {},
            std::string name = {}
        ) const = 0;

        [[nodiscard]] virtual ConfigurationReloadApiResult import_document(
            const dispatcher::config::ConfigurationDocument& document
        ) = 0;
    };
}