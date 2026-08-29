#include "dispatcher/data_hub/application.hpp"
#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/v1/data_hub.grpc.pb.h"
#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

int test_application() {
    using dispatcher::data_hub::Application;

    if (Application::service_name() != "data-hub") {
        return fail("unexpected service name");
    }

    std::ostringstream output;
    const Application application;

    if (application.run(output) != 0) {
        return fail("application returned non-zero exit code");
    }

    if (output.str() != "Dispatcher Data Hub\n") {
        return fail("unexpected application output");
    }

    return 0;
}

int test_contract() {
    namespace api = dispatcher::data_hub::v1;

    api::MetricSample sample;
    sample.mutable_metric_id()->set_value("AHU01.Temperature");
    sample.mutable_value()->set_double_value(23.5);
    sample.set_source_timestamp_unix_ms(1'700'000'000'000LL);

    if (sample.metric_id().value() != "AHU01.Temperature") {
        return fail("metric id was not preserved");
    }

    if (sample.value().kind_case() != api::MetricValue::kDoubleValue) {
        return fail("metric value type was not preserved");
    }

    if (sample.value().double_value() != 23.5) {
        return fail("metric value was not preserved");
    }

    std::string encoded;
    if (!sample.SerializeToString(&encoded)) {
        return fail("metric sample serialization failed");
    }

    api::MetricSample decoded;
    if (!decoded.ParseFromString(encoded)) {
        return fail("metric sample parsing failed");
    }

    if (decoded.metric_id().value() != sample.metric_id().value()) {
        return fail("decoded metric id differs from source");
    }

    api::SubscribeRequest subscription;
    subscription.add_metric_ids()->set_value("AHU01.Temperature");

    if (subscription.metric_ids_size() != 1) {
        return fail("subscription metric list is invalid");
    }

    api::WriteMetricRequest write_request;
    write_request.mutable_metric_id()->set_value("AHU01.Setpoint");
    write_request.mutable_value()->set_double_value(24.0);

    if (write_request.value().kind_case() != api::MetricValue::kDoubleValue) {
        return fail("write request value type is invalid");
    }

    [[maybe_unused]] api::DataHub::Service* grpc_service_type = nullptr;

    return 0;
}

int test_current_value_store() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::CurrentValueStore;

    CurrentValueStore store;

    if (store.size() != 0) {
        return fail("new current value store is not empty");
    }

    if (store.get("AHU01.Temperature").has_value()) {
        return fail("unknown metric unexpectedly has a current value");
    }

    api::MetricSample invalid_sample;
    invalid_sample.mutable_value()->set_double_value(1.0);

    if (store.put(invalid_sample)) {
        return fail("sample without metric id was accepted");
    }

    invalid_sample.Clear();
    invalid_sample.mutable_metric_id()->set_value("AHU01.Invalid");

    if (store.put(invalid_sample)) {
        return fail("sample without metric value was accepted");
    }

    api::MetricSample first;
    first.mutable_metric_id()->set_value("AHU01.Temperature");
    first.mutable_value()->set_double_value(22.1);
    first.set_source_timestamp_unix_ms(1000);

    if (!store.put(first)) {
        return fail("first current value was rejected");
    }

    const auto stored_first = store.get("AHU01.Temperature");
    if (!stored_first.has_value()) {
        return fail("stored current value cannot be retrieved");
    }

    if (stored_first->value().kind_case() != api::MetricValue::kDoubleValue ||
        stored_first->value().double_value() != 22.1 ||
        stored_first->source_timestamp_unix_ms() != 1000) {
        return fail("stored current value differs from published sample");
    }

    api::MetricSample replacement;
    replacement.mutable_metric_id()->set_value("AHU01.Temperature");
    replacement.mutable_value()->set_double_value(22.8);
    replacement.set_source_timestamp_unix_ms(2000);

    if (!store.put(replacement)) {
        return fail("replacement current value was rejected");
    }

    const auto stored_replacement = store.get("AHU01.Temperature");
    if (!stored_replacement.has_value() ||
        stored_replacement->value().double_value() != 22.8 ||
        stored_replacement->source_timestamp_unix_ms() != 2000) {
        return fail("replacement did not become the current value");
    }

    if (store.size() != 1) {
        return fail("replacing a metric unexpectedly increased store size");
    }

    api::MetricSample second_metric;
    second_metric.mutable_metric_id()->set_value("AHU01.Enabled");
    second_metric.mutable_value()->set_bool_value(true);

    if (!store.put(second_metric)) {
        return fail("second metric was rejected");
    }

    const auto stored_second = store.get("AHU01.Enabled");
    if (!stored_second.has_value() ||
        stored_second->value().kind_case() != api::MetricValue::kBoolValue ||
        !stored_second->value().bool_value()) {
        return fail("second metric current value is invalid");
    }

    if (store.size() != 2) {
        return fail("store does not contain two independent metrics");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_application(); result != 0) {
        return result;
    }

    if (const auto result = test_contract(); result != 0) {
        return result;
    }

    if (const auto result = test_current_value_store(); result != 0) {
        return result;
    }

    std::cout << "Data Hub application, contract and current value store tests passed\n";
    return 0;
}
