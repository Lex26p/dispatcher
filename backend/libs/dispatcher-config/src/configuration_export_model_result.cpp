#include <dispatcher/config/configuration_export_model_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::config
{
    ConfigurationExportModelResult ConfigurationExportModelResult::success(
        ConfigurationExportModel model
    )
    {
        return ConfigurationExportModelResult(
            ConfigurationIoResult::success(),
            std::move(model)
        );
    }

    ConfigurationExportModelResult ConfigurationExportModelResult::failure(
        ConfigurationIoStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationExportModelResult(
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

    bool ConfigurationExportModelResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationExportModelResult::failed() const noexcept
    {
        return result_.failed();
    }

    ConfigurationIoStatus ConfigurationExportModelResult::status()
        const noexcept
    {
        return result_.status();
    }

    const ConfigurationIoResult& ConfigurationExportModelResult::result()
        const noexcept
    {
        return result_;
    }

    const ConfigurationIoError& ConfigurationExportModelResult::error()
        const noexcept
    {
        return result_.error();
    }

    bool ConfigurationExportModelResult::has_model() const noexcept
    {
        return model_.has_value();
    }

    const ConfigurationExportModel& ConfigurationExportModelResult::model()
        const
    {
        if (!model_.has_value())
        {
            throw std::logic_error(
                "ConfigurationExportModelResult does not contain a model"
            );
        }

        return model_.value();
    }

    ConfigurationExportModelResult::ConfigurationExportModelResult(
        ConfigurationIoResult result,
        std::optional<ConfigurationExportModel> model
    )
        : result_(std::move(result))
        , model_(std::move(model))
    {
    }
}