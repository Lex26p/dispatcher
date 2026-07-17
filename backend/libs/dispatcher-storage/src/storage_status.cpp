#include <dispatcher/storage/storage_status.hpp>

namespace dispatcher::storage
{
    const char* to_string(StorageStatus status) noexcept
    {
        switch (status)
        {
        case StorageStatus::Success:
            return "success";
        case StorageStatus::NotFound:
            return "not_found";
        case StorageStatus::AlreadyExists:
            return "already_exists";
        case StorageStatus::Conflict:
            return "conflict";
        case StorageStatus::ValidationError:
            return "validation_error";
        case StorageStatus::SerializationError:
            return "serialization_error";
        case StorageStatus::IoError:
            return "io_error";
        case StorageStatus::BackendUnavailable:
            return "backend_unavailable";
        case StorageStatus::Timeout:
            return "timeout";
        case StorageStatus::UnsupportedOperation:
            return "unsupported_operation";
        case StorageStatus::UnknownError:
            return "unknown_error";
        }

        return "unknown_error";
    }

    bool is_success(StorageStatus status) noexcept
    {
        return status == StorageStatus::Success;
    }

    bool is_failure(StorageStatus status) noexcept
    {
        return !is_success(status);
    }
}