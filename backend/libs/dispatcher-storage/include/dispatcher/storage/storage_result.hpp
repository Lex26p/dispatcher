#pragma once

#include <dispatcher/storage/storage_error.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <string>

namespace dispatcher::storage
{
    class StorageResult
    {
    public:
        [[nodiscard]] static StorageResult success();

        [[nodiscard]] static StorageResult failure(
            StorageStatus status,
            std::string operation = {},
            std::string key = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] StorageStatus status() const noexcept;

        [[nodiscard]] const StorageError& error() const noexcept;

        [[nodiscard]] const std::string& operation() const noexcept;

        [[nodiscard]] const std::string& key() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

    private:
        explicit StorageResult(StorageError error);

        StorageError error_;
    };
}