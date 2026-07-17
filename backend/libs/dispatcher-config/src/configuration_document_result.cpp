#include <dispatcher/config/configuration_document_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::config
{
    ConfigurationDocumentResult ConfigurationDocumentResult::success(
        ConfigurationDocument document
    )
    {
        return ConfigurationDocumentResult(
            ConfigurationIoResult::success(),
            std::move(document)
        );
    }

    ConfigurationDocumentResult ConfigurationDocumentResult::failure(
        ConfigurationIoStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationDocumentResult(
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

    bool ConfigurationDocumentResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationDocumentResult::failed() const noexcept
    {
        return result_.failed();
    }

    ConfigurationIoStatus ConfigurationDocumentResult::status() const noexcept
    {
        return result_.status();
    }

    const ConfigurationIoResult& ConfigurationDocumentResult::result()
        const noexcept
    {
        return result_;
    }

    const ConfigurationIoError& ConfigurationDocumentResult::error()
        const noexcept
    {
        return result_.error();
    }

    bool ConfigurationDocumentResult::has_document() const noexcept
    {
        return document_.has_value();
    }

    const ConfigurationDocument& ConfigurationDocumentResult::document()
        const
    {
        if (!document_.has_value())
        {
            throw std::logic_error(
                "ConfigurationDocumentResult does not contain a document"
            );
        }

        return document_.value();
    }

    ConfigurationDocumentResult::ConfigurationDocumentResult(
        ConfigurationIoResult result,
        std::optional<ConfigurationDocument> document
    )
        : result_(std::move(result))
        , document_(std::move(document))
    {
    }
}