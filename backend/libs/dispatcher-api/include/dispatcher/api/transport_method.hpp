#pragma once

#include <string_view>

namespace dispatcher::api
{
    enum class TransportMethod
    {
        Unknown,
        Get,
        Post,
        Put,
        Patch,
        Delete
    };

    [[nodiscard]] const char* to_string(
        TransportMethod method
    ) noexcept;

    [[nodiscard]] TransportMethod transport_method_from_string(
        std::string_view method
    ) noexcept;

    [[nodiscard]] bool is_known_method(
        TransportMethod method
    ) noexcept;

    [[nodiscard]] bool is_read_method(
        TransportMethod method
    ) noexcept;

    [[nodiscard]] bool is_write_method(
        TransportMethod method
    ) noexcept;
}