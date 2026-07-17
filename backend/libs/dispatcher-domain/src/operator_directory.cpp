#include <dispatcher/domain/operator_directory.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::domain
{
    OperatorDirectoryResult OperatorDirectory::add(
        OperatorIdentity identity
    )
    {
        const auto operator_id = identity.operator_id().value();
        const auto username = identity.username();

        if (operator_id.empty())
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::InvalidIdentity,
                "operator id is empty",
                "operator_id",
                {}
            );
        }

        if (username.empty())
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::InvalidIdentity,
                "username is empty",
                "username",
                {}
            );
        }

        if (identities_by_id_.contains(operator_id))
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::DuplicateOperatorId,
                "operator id is already registered",
                "operator_id",
                operator_id
            );
        }

        if (id_by_username_.contains(username))
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::DuplicateUsername,
                "username is already registered",
                "username",
                username
            );
        }

        id_by_username_.emplace(
            username,
            operator_id
        );

        identities_by_id_.emplace(
            operator_id,
            std::move(identity)
        );

        return OperatorDirectoryResult::success(
            OperatorDirectoryStatus::Added,
            "operator identity added"
        );
    }

    OperatorDirectoryResult OperatorDirectory::remove(
        const OperatorId& operator_id
    )
    {
        const auto id_value = operator_id.value();

        if (id_value.empty())
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::InvalidIdentity,
                "operator id is empty",
                "operator_id",
                {}
            );
        }

        const auto iterator = identities_by_id_.find(id_value);

        if (iterator == identities_by_id_.end())
        {
            return OperatorDirectoryResult::failure(
                OperatorDirectoryStatus::NotFound,
                "operator identity not found",
                "operator_id",
                id_value
            );
        }

        id_by_username_.erase(
            iterator->second.username()
        );

        identities_by_id_.erase(iterator);

        return OperatorDirectoryResult::success(
            OperatorDirectoryStatus::Removed,
            "operator identity removed"
        );
    }

    std::optional<OperatorIdentity> OperatorDirectory::find_by_id(
        const OperatorId& operator_id
    ) const
    {
        const auto iterator =
            identities_by_id_.find(operator_id.value());

        if (iterator == identities_by_id_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    std::optional<OperatorIdentity> OperatorDirectory::find_by_username(
        const std::string& username
    ) const
    {
        const auto username_iterator = id_by_username_.find(username);

        if (username_iterator == id_by_username_.end())
        {
            return std::nullopt;
        }

        return find_by_id(
            OperatorId{ username_iterator->second }
        );
    }

    bool OperatorDirectory::contains_id(
        const OperatorId& operator_id
    ) const
    {
        return identities_by_id_.contains(operator_id.value());
    }

    bool OperatorDirectory::contains_username(
        const std::string& username
    ) const
    {
        return id_by_username_.contains(username);
    }

    std::vector<OperatorIdentity> OperatorDirectory::identities() const
    {
        std::vector<OperatorIdentity> result;

        result.reserve(identities_by_id_.size());

        for (const auto& [id, identity] : identities_by_id_)
        {
            result.push_back(identity);
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const OperatorIdentity& left, const OperatorIdentity& right)
            {
                return left.operator_id().value()
                    < right.operator_id().value();
            }
        );

        return result;
    }

    std::vector<std::string> OperatorDirectory::usernames() const
    {
        std::vector<std::string> result;

        result.reserve(id_by_username_.size());

        for (const auto& [username, id] : id_by_username_)
        {
            result.push_back(username);
        }

        std::sort(
            result.begin(),
            result.end()
        );

        return result;
    }

    std::size_t OperatorDirectory::size() const noexcept
    {
        return identities_by_id_.size();
    }

    bool OperatorDirectory::empty() const noexcept
    {
        return identities_by_id_.empty();
    }

    void OperatorDirectory::clear() noexcept
    {
        identities_by_id_.clear();
        id_by_username_.clear();
    }
}