#pragma once

#include <string_view>

namespace dispatcher::domain
{
    enum class DataType
    {
        Boolean,
        Int32,
        Int64,
        Float32,
        Float64,
        String
    };

    constexpr std::string_view to_string(DataType type)
    {
        switch (type)
        {
        case DataType::Boolean:
            return "boolean";
        case DataType::Int32:
            return "int32";
        case DataType::Int64:
            return "int64";
        case DataType::Float32:
            return "float32";
        case DataType::Float64:
            return "float64";
        case DataType::String:
            return "string";
        }

        return "unknown";
    }
}