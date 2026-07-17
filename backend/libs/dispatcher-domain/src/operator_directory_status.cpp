#include <dispatcher/domain/operator_directory_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(OperatorDirectoryStatus status) noexcept
    {
        switch (status)
        {
        case OperatorDirectoryStatus::Added:
            return "added";

        case OperatorDirectoryStatus::Removed:
            return "removed";

        case OperatorDirectoryStatus::NotFound:
            return "not_found";

        case OperatorDirectoryStatus::DuplicateOperatorId:
            return "duplicate_operator_id";

        case OperatorDirectoryStatus::DuplicateUsername:
            return "duplicate_username";

        case OperatorDirectoryStatus::InvalidIdentity:
            return "invalid_identity";
        }

        return "invalid_identity";
    }

    bool is_success(OperatorDirectoryStatus status) noexcept
    {
        switch (status)
        {
        case OperatorDirectoryStatus::Added:
        case OperatorDirectoryStatus::Removed:
            return true;

        case OperatorDirectoryStatus::NotFound:
        case OperatorDirectoryStatus::DuplicateOperatorId:
        case OperatorDirectoryStatus::DuplicateUsername:
        case OperatorDirectoryStatus::InvalidIdentity:
            return false;
        }

        return false;
    }

    bool is_failure(OperatorDirectoryStatus status) noexcept
    {
        return !is_success(status);
    }
}