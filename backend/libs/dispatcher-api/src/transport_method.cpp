#include <dispatcher/api/transport_method.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportMethod method) noexcept
    {
        switch (method)
        {
        case TransportMethod::Unknown:
            return "unknown";

        case TransportMethod::Get:
            return "GET";

        case TransportMethod::Post:
            return "POST";

        case TransportMethod::Put:
            return "PUT";

        case TransportMethod::Patch:
            return "PATCH";

        case TransportMethod::Delete:
            return "DELETE";
        }

        return "unknown";
    }

    TransportMethod transport_method_from_string(
        std::string_view method
    ) noexcept
    {
        if (method == "GET" || method == "get")
        {
            return TransportMethod::Get;
        }

        if (method == "POST" || method == "post")
        {
            return TransportMethod::Post;
        }

        if (method == "PUT" || method == "put")
        {
            return TransportMethod::Put;
        }

        if (method == "PATCH" || method == "patch")
        {
            return TransportMethod::Patch;
        }

        if (method == "DELETE" || method == "delete")
        {
            return TransportMethod::Delete;
        }

        return TransportMethod::Unknown;
    }

    bool is_known_method(TransportMethod method) noexcept
    {
        return method != TransportMethod::Unknown;
    }

    bool is_read_method(TransportMethod method) noexcept
    {
        return method == TransportMethod::Get;
    }

    bool is_write_method(TransportMethod method) noexcept
    {
        switch (method)
        {
        case TransportMethod::Post:
        case TransportMethod::Put:
        case TransportMethod::Patch:
        case TransportMethod::Delete:
            return true;

        case TransportMethod::Unknown:
        case TransportMethod::Get:
            return false;
        }

        return false;
    }
}