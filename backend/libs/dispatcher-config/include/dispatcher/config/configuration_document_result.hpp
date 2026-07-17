#pragma once

#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_io_result.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::config
{
    class ConfigurationDocumentResult
    {
    public:
        [[nodiscard]] static ConfigurationDocumentResult success(
            ConfigurationDocument document
        );

        [[nodiscard]] static ConfigurationDocumentResult failure(
            ConfigurationIoStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ConfigurationIoStatus status() const noexcept;

        [[nodiscard]] const ConfigurationIoResult& result() const noexcept;

        [[nodiscard]] const ConfigurationIoError& error() const noexcept;

        [[nodiscard]] bool has_document() const noexcept;

        [[nodiscard]] const ConfigurationDocument& document() const;

    private:
        ConfigurationDocumentResult(
            ConfigurationIoResult result,
            std::optional<ConfigurationDocument> document
        );

        ConfigurationIoResult result_;
        std::optional<ConfigurationDocument> document_;
    };
}