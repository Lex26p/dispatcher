#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_directory_result.hpp>
#include <dispatcher/domain/operator_identity.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::domain
{
    class OperatorDirectory
    {
    public:
        [[nodiscard]] OperatorDirectoryResult add(
            OperatorIdentity identity
        );

        [[nodiscard]] OperatorDirectoryResult remove(
            const OperatorId& operator_id
        );

        [[nodiscard]] std::optional<OperatorIdentity> find_by_id(
            const OperatorId& operator_id
        ) const;

        [[nodiscard]] std::optional<OperatorIdentity> find_by_username(
            const std::string& username
        ) const;

        [[nodiscard]] bool contains_id(
            const OperatorId& operator_id
        ) const;

        [[nodiscard]] bool contains_username(
            const std::string& username
        ) const;

        [[nodiscard]] std::vector<OperatorIdentity> identities() const;

        [[nodiscard]] std::vector<std::string> usernames() const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::unordered_map<std::string, OperatorIdentity> identities_by_id_;
        std::unordered_map<std::string, std::string> id_by_username_;
    };
}