#pragma once

#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_protocol.hpp>

#include <string>

namespace dispatcher::api
{
    class TransportEndpoint
    {
    public:
        TransportEndpoint(
            std::string name,
            TransportMethod method,
            std::string path,
            bool requires_authentication = true,
            bool streaming = false,
            std::string description = {}
        );

        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] TransportMethod method() const noexcept;

        [[nodiscard]] const std::string& path() const noexcept;

        [[nodiscard]] bool requires_authentication() const noexcept;

        [[nodiscard]] bool public_endpoint() const noexcept;

        [[nodiscard]] bool streaming() const noexcept;

        [[nodiscard]] const std::string& description() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool has_path() const noexcept;

        [[nodiscard]] bool has_description() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool matches(
            TransportMethod method,
            const std::string& path
        ) const noexcept;

        [[nodiscard]] bool compatible_with(
            TransportProtocol protocol
        ) const noexcept;

        [[nodiscard]] std::string key() const;

        [[nodiscard]] static std::string make_key(
            TransportMethod method,
            const std::string& path
        );

    private:
        std::string name_;
        TransportMethod method_{ TransportMethod::Unknown };
        std::string path_;
        bool requires_authentication_{ true };
        bool streaming_{ false };
        std::string description_;
    };
}