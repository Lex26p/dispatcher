#include "dispatcher/device_manager/domain.hpp"

#include <cctype>
#include <set>
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

[[nodiscard]] std::string association_key(
    const std::string_view resource_id,
    const std::string_view project_id) {
    std::string result;
    result.reserve(resource_id.size() + project_id.size() + 1);
    result.append(resource_id);
    result.push_back('\0');
    result.append(project_id);
    return result;
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

DomainValidationResult validate_catalog(const DeviceCatalog& catalog) {
    const auto base = validate_catalog(catalog.devices, catalog.metrics);
    if (!base.ok()) {
        return base;
    }

    std::unordered_set<std::string> device_ids;
    device_ids.reserve(catalog.devices.size());
    for (const auto& device : catalog.devices) {
        device_ids.insert(device.id);
    }

    std::unordered_map<std::string, const Metric*> metrics_by_id;
    metrics_by_id.reserve(catalog.metrics.size());
    for (const auto& metric : catalog.metrics) {
        metrics_by_id.emplace(metric.id, &metric);
    }

    std::unordered_set<std::string> device_associations;
    for (const auto& association : catalog.device_projects) {
        if (!valid_id(association.project_id)) {
            return DomainValidationResult::failure(
                DomainError::invalid_project_id,
                association.resource_id);
        }
        if (!device_ids.contains(association.resource_id)) {
            return DomainValidationResult::failure(
                DomainError::project_device_not_found,
                association.resource_id);
        }
        if (!device_associations.insert(
                association_key(association.resource_id, association.project_id))
                 .second) {
            return DomainValidationResult::failure(
                DomainError::duplicate_device_project_association,
                association.resource_id);
        }
    }

    std::unordered_set<std::string> standalone_associations;
    std::unordered_map<std::string, std::set<std::string>> standalone_projects;
    for (const auto& association : catalog.standalone_metric_projects) {
        if (!valid_id(association.project_id)) {
            return DomainValidationResult::failure(
                DomainError::invalid_project_id,
                association.resource_id);
        }
        const auto metric = metrics_by_id.find(association.resource_id);
        if (metric == metrics_by_id.end()) {
            return DomainValidationResult::failure(
                DomainError::project_metric_not_found,
                association.resource_id);
        }
        if (metric->second->device_id.has_value()) {
            return DomainValidationResult::failure(
                DomainError::project_metric_not_standalone,
                association.resource_id);
        }
        if (!standalone_associations.insert(
                association_key(association.resource_id, association.project_id))
                 .second) {
            return DomainValidationResult::failure(
                DomainError::duplicate_standalone_metric_project_association,
                association.resource_id);
        }
        standalone_projects[association.resource_id].insert(association.project_id);
    }

    for (const auto& metric : catalog.metrics) {
        if (metric.kind != MetricKind::working || metric.device_id.has_value()) {
            continue;
        }
        const auto& working_projects = standalone_projects[metric.id];
        const auto& state_projects = standalone_projects[*metric.state_metric_id];
        if (working_projects != state_projects) {
            return DomainValidationResult::failure(
                DomainError::standalone_state_project_mismatch,
                metric.id);
        }
    }

    return DomainValidationResult::success();
}

}  // namespace dispatcher::device_manager
