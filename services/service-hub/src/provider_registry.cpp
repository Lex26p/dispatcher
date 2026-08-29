#include "dispatcher/service_hub/provider_registry.hpp"

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace dispatcher::service_hub {
namespace {

[[nodiscard]] bool is_first_character_valid(const char value) noexcept {
    return (value >= 'a' && value <= 'z') ||
           (value >= '0' && value <= '9');
}

[[nodiscard]] bool is_character_valid(const char value) noexcept {
    return is_first_character_valid(value) ||
           value == '.' ||
           value == '_' ||
           value == '-';
}

}  // namespace

bool ProviderRegistry::is_valid_service_address(
    const std::string_view service) noexcept {
    if (service.empty() || service.size() > 128) {
        return false;
    }

    if (!is_first_character_valid(service.front())) {
        return false;
    }

    for (const char character : service) {
        if (!is_character_valid(character)) {
            return false;
        }
    }

    return true;
}

ProviderRegistry::RegisterResult ProviderRegistry::register_provider(
    const ProviderConnectionId connection_id,
    std::string service) {
    if (!is_valid_service_address(service)) {
        return RegisterResult::invalid_service;
    }

    std::unique_lock lock(mutex_);

    if (services_by_connection_.contains(connection_id)) {
        return RegisterResult::connection_already_registered;
    }

    if (providers_by_service_.contains(service)) {
        return RegisterResult::service_in_use;
    }

    const auto [service_iterator, service_inserted] =
        providers_by_service_.emplace(service, connection_id);

    if (!service_inserted) {
        return RegisterResult::service_in_use;
    }

    try {
        services_by_connection_.emplace(
            connection_id,
            std::move(service));
    } catch (...) {
        providers_by_service_.erase(service_iterator);
        throw;
    }

    return RegisterResult::registered;
}

bool ProviderRegistry::unregister_provider(
    const ProviderConnectionId connection_id) {
    std::unique_lock lock(mutex_);

    const auto connection_iterator =
        services_by_connection_.find(connection_id);

    if (connection_iterator == services_by_connection_.end()) {
        return false;
    }

    providers_by_service_.erase(connection_iterator->second);
    services_by_connection_.erase(connection_iterator);
    return true;
}

std::optional<ProviderConnectionId> ProviderRegistry::find_provider(
    const std::string_view service) const {
    std::shared_lock lock(mutex_);

    const auto provider_iterator =
        providers_by_service_.find(std::string(service));

    if (provider_iterator == providers_by_service_.end()) {
        return std::nullopt;
    }

    return provider_iterator->second;
}

std::optional<std::string> ProviderRegistry::find_service(
    const ProviderConnectionId connection_id) const {
    std::shared_lock lock(mutex_);

    const auto service_iterator =
        services_by_connection_.find(connection_id);

    if (service_iterator == services_by_connection_.end()) {
        return std::nullopt;
    }

    return service_iterator->second;
}

std::size_t ProviderRegistry::size() const {
    std::shared_lock lock(mutex_);
    return providers_by_service_.size();
}

}  // namespace dispatcher::service_hub
