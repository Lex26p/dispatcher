#pragma once

#include <dispatcher/storage/storage_status.hpp>

#include <string>

namespace dispatcher::storage
{
    struct StorageError
    {
        StorageStatus status{ StorageStatus::UnknownError };
        std::string operation;
        std::string key;
        std::string message;

        [[nodiscard]] bool empty() const noexcept
        {
            return status == StorageStatus::Success
                && operation.empty()
                && key.empty()
                && message.empty();
        }

        [[nodiscard]] bool has_message() const noexcept
        {
            return !message.empty();
        }

        [[nodiscard]] bool has_key() const noexcept
        {
            return !key.empty();
        }

        [[nodiscard]] bool has_operation() const noexcept
        {
            return !operation.empty();
        }
    };
}