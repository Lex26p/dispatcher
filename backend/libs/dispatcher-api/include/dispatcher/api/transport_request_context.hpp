#pragma once

#include <dispatcher/api/transport_protocol.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace dispatcher::api
{
    class TransportRequestContext
    {
    public:
        using Headers = std::unordered_map<std::string, std::string>;

        TransportRequestContext(
            TransportProtocol protocol,
            std::string method,
            std::string path,
            std::string correlation_id = {},
            std::string operator_id = {},
            std::string remote_address = {},
            Headers headers = {}
        );

        [[nodiscard]] TransportProtocol protocol() const noexcept;

        [[nodiscard]] const std::string& method() const noexcept;

        [[nodiscard]] const std::string& path() const noexcept;

        [[nodiscard]] const std::string& correlation_id() const noexcept;

        [[nodiscard]] const std::string& operator_id() const noexcept;

        [[nodiscard]] const std::string& remote_address() const noexcept;

        [[nodiscard]] const Headers& headers() const noexcept;

        [[nodiscard]] bool has_correlation_id() const noexcept;

        [[nodiscard]] bool has_operator_id() const noexcept;

        [[nodiscard]] bool has_remote_address() const noexcept;

        [[nodiscard]] bool has_headers() const noexcept;

        [[nodiscard]] bool has_header(
            const std::string& name
        ) const;

        [[nodiscard]] std::optional<std::string> header(
            const std::string& name
        ) const;

        [[nodiscard]] std::size_t header_count() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

    private:
        TransportProtocol protocol_{ TransportProtocol::Unknown };
        std::string method_;
        std::string path_;
        std::string correlation_id_;
        std::string operator_id_;
        std::string remote_address_;
        Headers headers_;
    };
}