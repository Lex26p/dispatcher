#pragma once

#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_import_model_result.hpp>

namespace dispatcher::config
{
    class ConfigurationJsonImportParser
    {
    public:
        [[nodiscard]] static ConfigurationImportModelResult parse(
            const ConfigurationDocument& document
        );
    };
}