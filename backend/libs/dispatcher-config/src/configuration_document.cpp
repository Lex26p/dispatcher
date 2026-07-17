#include <dispatcher/config/configuration_document.hpp>

#include <utility>

namespace dispatcher::config
{
    ConfigurationDocument::ConfigurationDocument(
        ConfigurationFormat format,
        std::string content,
        std::string name
    )
        : format_(format)
        , content_(std::move(content))
        , name_(std::move(name))
    {
    }

    ConfigurationDocument ConfigurationDocument::json(
        std::string content,
        std::string name
    )
    {
        return ConfigurationDocument(
            ConfigurationFormat::Json,
            std::move(content),
            std::move(name)
        );
    }

    ConfigurationFormat ConfigurationDocument::format() const noexcept
    {
        return format_;
    }

    const std::string& ConfigurationDocument::content() const noexcept
    {
        return content_;
    }

    const std::string& ConfigurationDocument::name() const noexcept
    {
        return name_;
    }

    std::size_t ConfigurationDocument::size() const noexcept
    {
        return content_.size();
    }

    bool ConfigurationDocument::empty() const noexcept
    {
        return content_.empty();
    }

    bool ConfigurationDocument::has_name() const noexcept
    {
        return !name_.empty();
    }

    bool ConfigurationDocument::has_known_format() const noexcept
    {
        return is_known_configuration_format(format_);
    }
}