#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace dispatcher::device_manager {

inline constexpr std::size_t opaque_id_max_bytes = 256;
inline constexpr std::size_t name_max_bytes = 256;
inline constexpr std::size_t description_max_bytes = 4096;
inline constexpr std::size_t location_max_bytes = 1024;
inline constexpr std::size_t unit_max_bytes = 128;

enum class MetricValueType {
    boolean,
    signed_integer,
    unsigned_integer,
    floating_point,
    string,
    bytes,
};

enum class MetricKind {
    working,
    state,
};

struct Device final {
    std::string id;
    std::string name;
    std::string description;
    std::string location;

    friend bool operator==(const Device&, const Device&) = default;
};

struct Metric final {
    std::string id;
    std::optional<std::string> device_id;
    std::string name;
    std::string description;
    MetricValueType value_type{MetricValueType::floating_point};
    std::string unit;
    bool writable{false};
    MetricKind kind{MetricKind::working};
    std::optional<std::string> state_metric_id;

    friend bool operator==(const Metric&, const Metric&) = default;
};

enum class DomainError {
    none,
    invalid_id,
    name_required,
    name_too_long,
    description_too_long,
    location_too_long,
    unit_too_long,
    invalid_device_id,
    invalid_state_metric_id,
    duplicate_device_id,
    duplicate_metric_id,
    device_not_found,
    state_metric_required,
    state_metric_link_forbidden,
    state_metric_self_reference,
    state_metric_writable,
    state_metric_not_found,
    state_metric_role_required,
    state_metric_device_mismatch,
};

struct DomainValidationResult final {
    DomainError error{DomainError::none};
    std::string subject_id;

    [[nodiscard]] bool ok() const noexcept {
        return error == DomainError::none;
    }

    [[nodiscard]] static DomainValidationResult success() {
        return {};
    }

    [[nodiscard]] static DomainValidationResult failure(
        DomainError error,
        std::string subject_id = {});
};

[[nodiscard]] DomainValidationResult validate_device(const Device& device);

[[nodiscard]] DomainValidationResult validate_metric_shape(const Metric& metric);

[[nodiscard]] DomainValidationResult validate_catalog(
    std::span<const Device> devices,
    std::span<const Metric> metrics);

[[nodiscard]] constexpr std::string_view metric_value_type_name(
    const MetricValueType type) noexcept {
    switch (type) {
    case MetricValueType::boolean:
        return "bool";
    case MetricValueType::signed_integer:
        return "int64";
    case MetricValueType::unsigned_integer:
        return "uint64";
    case MetricValueType::floating_point:
        return "double";
    case MetricValueType::string:
        return "string";
    case MetricValueType::bytes:
        return "bytes";
    }
    return "unknown";
}

}  // namespace dispatcher::device_manager
