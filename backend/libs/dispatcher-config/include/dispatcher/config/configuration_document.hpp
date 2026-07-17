#pragma once

#include <dispatcher/config/configuration_format.hpp>

#include <cstddef>
#include <string>

namespace dispatcher::config
{
    class ConfigurationDocument
    {
    public:
        ConfigurationDocument() = default;

        ConfigurationDocument(
            ConfigurationFormat format,
            std::string content,
            std::string name = {}
        );

        [[nodiscard]] static ConfigurationDocument json(
            std::string content,
            std::string name = {}
        );

        [[nodiscard]] ConfigurationFormat format() const noexcept;

        [[nodiscard]] const std::string& content() const noexcept;

        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool has_known_format() const noexcept;

    private:
        ConfigurationFormat format_{ ConfigurationFormat::Unknown };
        std::string content_;
        std::string name_;
    };
}