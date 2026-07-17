#include <dispatcher/config/configuration_import_model_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::config
{
    ConfigurationImportModelResult ConfigurationImportModelResult::success(
        ConfigurationImportModel model
    )
    {
        return ConfigurationImportModelResult(
            ConfigurationIoResult::success(),
            std::move(model)
        );
    }

    ConfigurationImportModelResult ConfigurationImportModelResult::failure(
        ConfigurationIoStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationImportModelResult(
            ConfigurationIoResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool ConfigurationImportModelResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationImportModelResult::failed() const noexcept
    {
        return result_.failed();
    }

    ConfigurationIoStatus ConfigurationImportModelResult::status()
        const noexcept
    {
        return result_.status();
    }

    const ConfigurationIoResult& ConfigurationImportModelResult::result()
        const noexcept
    {
        return result_;
    }

    const ConfigurationIoError& ConfigurationImportModelResult::error()
        const noexcept
    {
        return result_.error();
    }

    bool ConfigurationImportModelResult::has_model() const noexcept
    {
        return model_.has_value();
    }

    const ConfigurationImportModel& ConfigurationImportModelResult::model()
        const
    {
        if (!model_.has_value())
        {
            throw std::logic_error(
                "ConfigurationImportModelResult does not contain a model"
            );
        }

        return model_.value();
    }

    ConfigurationImportModelResult::ConfigurationImportModelResult(
        ConfigurationIoResult result,
        std::optional<ConfigurationImportModel> model
    )
        : result_(std::move(result))
        , model_(std::move(model))
    {
    }
}