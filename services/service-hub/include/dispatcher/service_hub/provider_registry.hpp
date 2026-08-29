#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace dispatcher::service_hub {

using ProviderConnectionId = std::uint64_t;

class ProviderRegistry final {
public:
    enum class RegisterResult {
        registered,
        invalid_service,
        service_in_use,
        connection_already_registered,
    };

    [[nodiscard]] static bool is_valid_service_address(
        std::string_view service) noexcept;

    [[nodiscard]] RegisterResult register_provider(
        ProviderConnectionId connection_id,
        std::string service);

    [[nodiscard]] bool unregister_provider(
        ProviderConnectionId connection_id);

    [[nodiscard]] std::optional<ProviderConnectionId> find_provider(
        std::string_view service) const;

    [[nodiscard]] std::optional<std::string> find_service(
        ProviderConnectionId connection_id) const;

    [[nodiscard]] std::size_t size() const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, ProviderConnectionId> providers_by_service_;
    std::unordered_map<ProviderConnectionId, std::string> services_by_connection_;
};

}  // namespace dispatcher::service_hub
