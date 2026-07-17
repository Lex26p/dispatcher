#include <dispatcher/storage/storage_result.hpp>

#include <utility>

namespace dispatcher::storage
{
    StorageResult StorageResult::success()
    {
        return StorageResult(
            StorageError{
                .status = StorageStatus::Success
            }
        );
    }

    StorageResult StorageResult::failure(
        StorageStatus status,
        std::string operation,
        std::string key,
        std::string message
    )
    {
        if (status == StorageStatus::Success)
        {
            status = StorageStatus::UnknownError;
        }

        return StorageResult(
            StorageError{
                .status = status,
                .operation = std::move(operation),
                .key = std::move(key),
                .message = std::move(message)
            }
        );
    }

    bool StorageResult::ok() const noexcept
    {
        return error_.status == StorageStatus::Success;
    }

    bool StorageResult::failed() const noexcept
    {
        return !ok();
    }

    StorageStatus StorageResult::status() const noexcept
    {
        return error_.status;
    }

    const StorageError& StorageResult::error() const noexcept
    {
        return error_;
    }

    const std::string& StorageResult::operation() const noexcept
    {
        return error_.operation;
    }

    const std::string& StorageResult::key() const noexcept
    {
        return error_.key;
    }

    const std::string& StorageResult::message() const noexcept
    {
        return error_.message;
    }

    StorageResult::StorageResult(StorageError error)
        : error_(std::move(error))
    {
    }
}