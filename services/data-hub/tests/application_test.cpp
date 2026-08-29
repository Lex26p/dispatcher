#include "dispatcher/data_hub/application.hpp"
#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/server.hpp"
#include "dispatcher/data_hub/v1/data_hub.grpc.pb.h"
#include "dispatcher/data_hub/v1/data_hub.pb.h"
#include "dispatcher/data_hub/write_router.hpp"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace {

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

bool wait_for_channel(
    const std::shared_ptr<grpc::Channel>& channel) {
    return channel->WaitForConnected(
        std::chrono::system_clock::now() + std::chrono::seconds(5));
}

int publish_double(
    dispatcher::data_hub::v1::DataHub::Stub& stub,
    std::string_view metric_id,
    double value,
    std::int64_t timestamp) {
    namespace api = dispatcher::data_hub::v1;

    api::PublishMetricRequest request;
    request.mutable_sample()->mutable_metric_id()->set_value(
        std::string(metric_id));
    request.mutable_sample()->mutable_value()->set_double_value(value);
    request.mutable_sample()->set_source_timestamp_unix_ms(timestamp);

    api::PublishMetricResponse response;
    grpc::ClientContext context;

    const auto status = stub.PublishMetric(
        &context,
        request,
        &response);

    return status.ok() ? 0 : 1;
}

int publish_string(
    dispatcher::data_hub::v1::DataHub::Stub& stub,
    std::string_view metric_id,
    std::string_view value,
    std::int64_t timestamp) {
    namespace api = dispatcher::data_hub::v1;

    api::PublishMetricRequest request;
    request.mutable_sample()->mutable_metric_id()->set_value(
        std::string(metric_id));
    request.mutable_sample()->mutable_value()->set_string_value(
        std::string(value));
    request.mutable_sample()->set_source_timestamp_unix_ms(timestamp);

    api::PublishMetricResponse response;
    grpc::ClientContext context;

    const auto status = stub.PublishMetric(
        &context,
        request,
        &response);

    return status.ok() ? 0 : 1;
}

class RecordingWriteProvider final
    : public dispatcher::data_hub::MetricWriteProvider {
public:
    explicit RecordingWriteProvider(bool accept = true)
        : accept_(accept) {}

    bool write(
        const dispatcher::data_hub::v1::WriteMetricRequest& request) override {
        std::lock_guard lock(mutex_);
        requests_.push_back(request);
        return accept_;
    }

    [[nodiscard]] std::size_t request_count() const {
        std::lock_guard lock(mutex_);
        return requests_.size();
    }

    [[nodiscard]] dispatcher::data_hub::v1::WriteMetricRequest
    last_request() const {
        std::lock_guard lock(mutex_);

        if (requests_.empty()) {
            return {};
        }

        return requests_.back();
    }

private:
    bool accept_;
    mutable std::mutex mutex_;
    std::vector<dispatcher::data_hub::v1::WriteMetricRequest> requests_;
};

int test_application_metadata() {
    using dispatcher::data_hub::Application;

    if (Application::service_name() != "data-hub") {
        return fail("unexpected service name");
    }

    return 0;
}

int test_current_value_store() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::CurrentValueStore;

    CurrentValueStore store;

    api::MetricSample first;
    first.mutable_metric_id()->set_value("AHU01.Temperature");
    first.mutable_value()->set_double_value(22.1);

    if (!store.put(first)) {
        return fail("current value store rejected a valid sample");
    }

    api::MetricSample replacement;
    replacement.mutable_metric_id()->set_value("AHU01.Temperature");
    replacement.mutable_value()->set_double_value(22.8);

    if (!store.put(replacement)) {
        return fail("current value store rejected a replacement");
    }

    const auto stored = store.get("AHU01.Temperature");

    if (!stored.has_value() ||
        stored->value().kind_case() != api::MetricValue::kDoubleValue ||
        stored->value().double_value() != 22.8 ||
        store.size() != 1) {
        return fail("current value store replacement behavior is invalid");
    }

    return 0;
}

int test_state_metrics_over_grpc() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view working_metric = "AHU01.Temperature";
    constexpr std::string_view state_metric = "AHU01.Temperature.State";

    DataHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("state-metric test server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(channel)) {
        server.shutdown();
        return fail("state-metric client failed to connect");
    }

    auto stub = api::DataHub::NewStub(channel);

    if (publish_double(*stub, working_metric, 25.0, 1000) != 0 ||
        publish_string(*stub, state_metric, "Warning", 1100) != 0) {
        server.shutdown();
        return fail("working/state metric publication failed");
    }

    api::GetCurrentRequest state_get;
    state_get.mutable_metric_id()->set_value(std::string(state_metric));

    api::GetCurrentResponse state_response;
    grpc::ClientContext state_context;

    const auto state_status = stub->GetCurrent(
        &state_context,
        state_get,
        &state_response);

    if (!state_status.ok() ||
        !state_response.has_sample() ||
        state_response.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        state_response.sample().value().string_value() != "Warning") {
        server.shutdown();
        return fail("state metric GetCurrent result is invalid");
    }

    server.shutdown();
    return 0;
}

int test_write_routing_over_grpc() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view setpoint_metric = "AHU01.Setpoint";

    auto provider = std::make_shared<RecordingWriteProvider>();

    DataHubServer server("127.0.0.1:0");

    if (!server.register_write_provider(
            std::string(setpoint_metric),
            provider)) {
        return fail("write provider registration failed");
    }

    if (server.register_write_provider(
            std::string(setpoint_metric),
            std::make_shared<RecordingWriteProvider>())) {
        return fail("duplicate write provider registration was accepted");
    }

    if (!server.start()) {
        return fail("write-routing test server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(channel)) {
        server.shutdown();
        return fail("write-routing client failed to connect");
    }

    auto stub = api::DataHub::NewStub(channel);

    // Publish the last confirmed physical/runtime value first.
    if (publish_double(*stub, setpoint_metric, 22.0, 1000) != 0) {
        server.shutdown();
        return fail("initial setpoint publication failed");
    }

    api::WriteMetricRequest write_request;
    write_request.mutable_metric_id()->set_value(
        std::string(setpoint_metric));
    write_request.mutable_value()->set_double_value(24.0);

    api::WriteMetricResponse write_response;
    grpc::ClientContext write_context;

    const auto write_status = stub->WriteMetric(
        &write_context,
        write_request,
        &write_response);

    if (!write_status.ok()) {
        server.shutdown();
        return fail("WriteMetric RPC failed");
    }

    if (provider->request_count() != 1) {
        server.shutdown();
        return fail("write provider did not receive exactly one request");
    }

    const auto received = provider->last_request();

    if (!received.has_metric_id() ||
        received.metric_id().value() != setpoint_metric ||
        !received.has_value() ||
        received.value().kind_case() != api::MetricValue::kDoubleValue ||
        received.value().double_value() != 24.0) {
        server.shutdown();
        return fail("write provider received an unexpected request");
    }

    // WriteMetric is a control request, not confirmation of the actual value.
    // Data Hub must keep the last published current value until a source
    // publishes a new confirmed value.
    api::GetCurrentRequest current_request;
    current_request.mutable_metric_id()->set_value(
        std::string(setpoint_metric));

    api::GetCurrentResponse current_response;
    grpc::ClientContext current_context;

    const auto current_status = stub->GetCurrent(
        &current_context,
        current_request,
        &current_response);

    if (!current_status.ok() ||
        !current_response.has_sample() ||
        current_response.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        current_response.sample().value().double_value() != 22.0) {
        server.shutdown();
        return fail("WriteMetric incorrectly changed the current value");
    }

    api::WriteMetricRequest unowned_request;
    unowned_request.mutable_metric_id()->set_value("AHU01.Unowned");
    unowned_request.mutable_value()->set_bool_value(true);

    api::WriteMetricResponse unowned_response;
    grpc::ClientContext unowned_context;

    const auto unowned_status = stub->WriteMetric(
        &unowned_context,
        unowned_request,
        &unowned_response);

    if (unowned_status.error_code() != grpc::StatusCode::NOT_FOUND) {
        server.shutdown();
        return fail("unowned metric did not return NOT_FOUND");
    }

    api::WriteMetricRequest invalid_request;
    invalid_request.mutable_metric_id()->set_value(
        std::string(setpoint_metric));

    api::WriteMetricResponse invalid_response;
    grpc::ClientContext invalid_context;

    const auto invalid_status = stub->WriteMetric(
        &invalid_context,
        invalid_request,
        &invalid_response);

    if (invalid_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT) {
        server.shutdown();
        return fail("invalid write request did not return INVALID_ARGUMENT");
    }

    auto rejecting_provider =
        std::make_shared<RecordingWriteProvider>(false);

    if (!server.register_write_provider(
            "AHU01.Rejected",
            rejecting_provider)) {
        server.shutdown();
        return fail("rejecting provider registration failed");
    }

    api::WriteMetricRequest rejected_request;
    rejected_request.mutable_metric_id()->set_value("AHU01.Rejected");
    rejected_request.mutable_value()->set_bool_value(true);

    api::WriteMetricResponse rejected_response;
    grpc::ClientContext rejected_context;

    const auto rejected_status = stub->WriteMetric(
        &rejected_context,
        rejected_request,
        &rejected_response);

    if (rejected_status.error_code() !=
        grpc::StatusCode::FAILED_PRECONDITION) {
        server.shutdown();
        return fail("provider rejection did not return FAILED_PRECONDITION");
    }

    server.shutdown();
    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_application_metadata(); result != 0) {
        return result;
    }

    if (const auto result = test_current_value_store(); result != 0) {
        return result;
    }

    if (const auto result = test_state_metrics_over_grpc(); result != 0) {
        return result;
    }

    if (const auto result = test_write_routing_over_grpc(); result != 0) {
        return result;
    }

    std::cout << "Data Hub write-routing tests passed\n";
    return 0;
}
