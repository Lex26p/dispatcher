#include "dispatcher/device_manager/domain.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace dm = dispatcher::device_manager;

namespace {

[[noreturn]] void fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

void expect_error(
    const dm::DomainValidationResult& result,
    const dm::DomainError error,
    const std::string_view message) {
    expect(!result.ok() && result.error == error, message);
}

[[nodiscard]] dm::Metric state_metric(
    std::string id,
    std::optional<std::string> device_id) {
    return dm::Metric{
        .id = std::move(id),
        .device_id = std::move(device_id),
        .name = "State",
        .description = {},
        .value_type = dm::MetricValueType::string,
        .unit = {},
        .writable = false,
        .kind = dm::MetricKind::state,
        .state_metric_id = std::nullopt,
    };
}

[[nodiscard]] dm::Metric working_metric(
    std::string id,
    std::optional<std::string> device_id,
    std::string state_id,
    const bool writable = false) {
    return dm::Metric{
        .id = std::move(id),
        .device_id = std::move(device_id),
        .name = "Temperature",
        .description = "Supply temperature",
        .value_type = dm::MetricValueType::floating_point,
        .unit = "degC",
        .writable = writable,
        .kind = dm::MetricKind::working,
        .state_metric_id = std::move(state_id),
    };
}

void test_valid_device_and_metric_catalog() {
    const std::vector<dm::Device> devices{
        dm::Device{
            .id = "device-1",
            .name = "Air handler",
            .description = "Main AHU",
            .location = "Plant room",
        },
    };
    const std::vector<dm::Metric> metrics{
        state_metric("metric-state-1", "device-1"),
        working_metric("metric-temperature-1", "device-1", "metric-state-1"),
    };

    expect(dm::validate_catalog(devices, metrics).ok(), "valid catalog should pass");
}

void test_standalone_metric_pair_is_valid() {
    const std::vector<dm::Device> devices;
    const std::vector<dm::Metric> metrics{
        state_metric("system-load-state", std::nullopt),
        working_metric("system-load", std::nullopt, "system-load-state"),
    };

    expect(
        dm::validate_catalog(devices, metrics).ok(),
        "standalone working/state metric pair should pass");
}

void test_value_type_model_matches_data_hub_baseline() {
    constexpr std::array types{
        dm::MetricValueType::boolean,
        dm::MetricValueType::signed_integer,
        dm::MetricValueType::unsigned_integer,
        dm::MetricValueType::floating_point,
        dm::MetricValueType::string,
        dm::MetricValueType::bytes,
    };
    constexpr std::array<std::string_view, 6> names{
        "bool", "int64", "uint64", "double", "string", "bytes"};

    for (std::size_t index = 0; index < types.size(); ++index) {
        expect(
            dm::metric_value_type_name(types[index]) == names[index],
            "value type name should match Data Hub semantic type");
    }
}

void test_state_metric_invariants() {
    auto writable_state = state_metric("state", std::nullopt);
    writable_state.writable = true;
    expect_error(
        dm::validate_metric_shape(writable_state),
        dm::DomainError::state_metric_writable,
        "state metric must not be writable");

    auto linked_state = state_metric("state", std::nullopt);
    linked_state.state_metric_id = "another-state";
    expect_error(
        dm::validate_metric_shape(linked_state),
        dm::DomainError::state_metric_link_forbidden,
        "state metric must not have its own state link");

    auto missing_link = working_metric("working", std::nullopt, "state");
    missing_link.state_metric_id.reset();
    expect_error(
        dm::validate_metric_shape(missing_link),
        dm::DomainError::state_metric_required,
        "working metric must have state link");
}

void test_catalog_referential_integrity() {
    const std::vector<dm::Device> devices{
        dm::Device{.id = "device-a", .name = "A", .description = {}, .location = {}},
        dm::Device{.id = "device-b", .name = "B", .description = {}, .location = {}},
    };

    expect_error(
        dm::validate_catalog(
            devices,
            std::vector<dm::Metric>{
                working_metric("working", "device-a", "missing-state")}),
        dm::DomainError::state_metric_not_found,
        "dangling state link must fail");

    expect_error(
        dm::validate_catalog(
            devices,
            std::vector<dm::Metric>{
                state_metric("state", "device-a"),
                working_metric("working", "device-b", "state")}),
        dm::DomainError::state_metric_device_mismatch,
        "working and state metrics must use the same device association");

    expect_error(
        dm::validate_catalog(
            std::vector<dm::Device>{},
            std::vector<dm::Metric>{
                state_metric("state", "missing-device")}),
        dm::DomainError::device_not_found,
        "metric must not reference unknown device");

    expect_error(
        dm::validate_catalog(
            std::vector<dm::Device>{},
            std::vector<dm::Metric>{
                state_metric("target-state", std::nullopt),
                working_metric("target-working", std::nullopt, "target-state"),
                working_metric("source-working", std::nullopt, "target-working")}),
        dm::DomainError::state_metric_role_required,
        "working metric must link to a state metric, not another working metric");
}

void test_identity_and_metadata_validation() {
    expect_error(
        dm::validate_device(dm::Device{.id = "device", .name = "   ", .description = {}, .location = {}}),
        dm::DomainError::name_required,
        "device name must contain non-whitespace");

    expect_error(
        dm::validate_device(dm::Device{
            .id = "device",
            .name = "Device",
            .description = std::string(dm::description_max_bytes + 1, 'x'),
            .location = {},
        }),
        dm::DomainError::description_too_long,
        "device description byte limit must be enforced");

    expect_error(
        dm::validate_catalog(
            std::vector<dm::Device>{
                dm::Device{.id = "duplicate", .name = "A", .description = {}, .location = {}},
                dm::Device{.id = "duplicate", .name = "B", .description = {}, .location = {}}},
            std::vector<dm::Metric>{}),
        dm::DomainError::duplicate_device_id,
        "duplicate device ids must fail");

    expect_error(
        dm::validate_catalog(
            std::vector<dm::Device>{},
            std::vector<dm::Metric>{
                state_metric("duplicate", std::nullopt),
                state_metric("duplicate", std::nullopt)}),
        dm::DomainError::duplicate_metric_id,
        "duplicate metric ids must fail");
}

}  // namespace

int main() {
    test_valid_device_and_metric_catalog();
    test_standalone_metric_pair_is_valid();
    test_value_type_model_matches_data_hub_baseline();
    test_state_metric_invariants();
    test_catalog_referential_integrity();
    test_identity_and_metadata_validation();
    std::cout << "Device Manager domain tests passed\n";
    return 0;
}
