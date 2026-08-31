#include "dispatcher/device_manager/domain.hpp"

#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace dispatcher::device_manager {
namespace {

[[nodiscard]] bool contains_non_whitespace(const std::string_view value) noexcept {
    for (const unsigned char character : value) {
        if (std::isspace(character) == 0) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool valid_id(const std::string_view value) noexcept {
    return !value.empty() &&
        value.size() <= opaque_id_max_bytes &&
        contains_non_whitespace(value);
}

}  // namespace

DomainValidationResult DomainValidationResult::failure(
    const DomainError error,
    std::string subject_id) {
    return DomainValidationResult{
        .error = error,
        .subject_id = std::move(subject_id),
    };
}

DomainValidationResult validate_device(const Device& device) {
    if (!valid_id(device.id)) {
        return DomainValidationResult::failure(DomainError::invalid_id, device.id);
    }
    if (!contains_non_whitespace(device.name)) {
        return DomainValidationResult::failure(DomainError::name_required, device.id);
    }
    if (device.name.size() > name_max_bytes) {
        return DomainValidationResult::failure(DomainError::name_too_long, device.id);
    }
    if (device.description.size() > description_max_bytes) {
        return DomainValidationResult::failure(
            DomainError::description_too_long,
            device.id);
    }
    if (device.location.size() > location_max_bytes) {
        return DomainValidationResult::failure(DomainError::location_too_long, device.id);
    }
    return DomainValidationResult::success();
}

DomainValidationResult validate_metric_shape(const Metric& metric) {
    if (!valid_id(metric.id)) {
        return DomainValidationResult::failure(DomainError::invalid_id, metric.id);
    }
    if (metric.device_id.has_value() && !valid_id(*metric.device_id)) {
        return DomainValidationResult::failure(
            DomainError::invalid_device_id,
            metric.id);
    }
    if (!contains_non_whitespace(metric.name)) {
        return DomainValidationResult::failure(DomainError::name_required, metric.id);
    }
    if (metric.name.size() > name_max_bytes) {
        return DomainValidationResult::failure(DomainError::name_too_long, metric.id);
    }
    if (metric.description.size() > description_max_bytes) {
        return DomainValidationResult::failure(
            DomainError::description_too_long,
            metric.id);
    }
    if (metric.unit.size() > unit_max_bytes) {
        return DomainValidationResult::failure(DomainError::unit_too_long, metric.id);
    }

    if (metric.kind == MetricKind::state) {
        if (metric.writable) {
            return DomainValidationResult::failure(
                DomainError::state_metric_writable,
                metric.id);
        }
        if (metric.state_metric_id.has_value()) {
            return DomainValidationResult::failure(
                DomainError::state_metric_link_forbidden,
                metric.id);
        }
        return DomainValidationResult::success();
    }

    if (!metric.state_metric_id.has_value()) {
        return DomainValidationResult::failure(
            DomainError::state_metric_required,
            metric.id);
    }
    if (!valid_id(*metric.state_metric_id)) {
        return DomainValidationResult::failure(
            DomainError::invalid_state_metric_id,
            metric.id);
    }
    if (*metric.state_metric_id == metric.id) {
        return DomainValidationResult::failure(
            DomainError::state_metric_self_reference,
            metric.id);
    }
    return DomainValidationResult::success();
}

DomainValidationResult validate_catalog(
    const std::span<const Device> devices,
    const std::span<const Metric> metrics) {
    std::unordered_set<std::string> device_ids;
    device_ids.reserve(devices.size());

    for (const auto& device : devices) {
        const auto validation = validate_device(device);
        if (!validation.ok()) {
            return validation;
        }
        if (!device_ids.insert(device.id).second) {
            return DomainValidationResult::failure(
                DomainError::duplicate_device_id,
                device.id);
        }
    }

    std::unordered_map<std::string, const Metric*> metrics_by_id;
    metrics_by_id.reserve(metrics.size());

    for (const auto& metric : metrics) {
        const auto validation = validate_metric_shape(metric);
        if (!validation.ok()) {
            return validation;
        }
        if (metric.device_id.has_value() &&
            !device_ids.contains(*metric.device_id)) {
            return DomainValidationResult::failure(
                DomainError::device_not_found,
                metric.id);
        }
        if (!metrics_by_id.emplace(metric.id, &metric).second) {
            return DomainValidationResult::failure(
                DomainError::duplicate_metric_id,
                metric.id);
        }
    }

    for (const auto& metric : metrics) {
        if (metric.kind != MetricKind::working) {
            continue;
        }

        const auto state = metrics_by_id.find(*metric.state_metric_id);
        if (state == metrics_by_id.end()) {
            return DomainValidationResult::failure(
                DomainError::state_metric_not_found,
                metric.id);
        }
        if (state->second->kind != MetricKind::state) {
            return DomainValidationResult::failure(
                DomainError::state_metric_role_required,
                metric.id);
        }
        if (state->second->device_id != metric.device_id) {
            return DomainValidationResult::failure(
                DomainError::state_metric_device_mismatch,
                metric.id);
        }
    }

    return DomainValidationResult::success();
}

}  // namespace dispatcher::device_manager
