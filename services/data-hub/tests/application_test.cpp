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

std::unique_ptr<dispatcher::data_hub::v1::DataHub::Stub> make_stub(
    const std::shared_ptr<grpc::Channel>& channel) {
    return dispatcher::data_hub::v1::DataHub::NewStub(channel);
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

    auto stub = make_stub(channel);

    if (publish_double(*stub, "AHU01.Temperature", 25.0, 1000) != 0 ||
        publish_string(
            *stub,
            "AHU01.Temperature.State",
            "Warning",
            1100) != 0) {
        server.shutdown();
        return fail("working/state metric publication failed");
    }

    api::GetCurrentRequest request;
    request.mutable_metric_id()->set_value("AHU01.Temperature.State");

    api::GetCurrentResponse response;
    grpc::ClientContext context;

    const auto status = stub->GetCurrent(
        &context,
        request,
        &response);

    if (!status.ok() ||
        !response.has_sample() ||
        response.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        response.sample().value().string_value() != "Warning") {
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

    auto stub = make_stub(channel);

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

    if (!write_status.ok() || provider->request_count() != 1) {
        server.shutdown();
        return fail("WriteMetric was not delivered to the provider");
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

    server.shutdown();
    return 0;
}

int test_rpc_errors() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    DataHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("RPC-error test server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(channel)) {
        server.shutdown();
        return fail("RPC-error client failed to connect");
    }

    auto stub = make_stub(channel);

    api::PublishMetricRequest invalid_publish;
    invalid_publish.mutable_sample()->mutable_metric_id()->set_value(
        "AHU01.Invalid");

    api::PublishMetricResponse invalid_publish_response;
    grpc::ClientContext invalid_publish_context;

    const auto publish_status = stub->PublishMetric(
        &invalid_publish_context,
        invalid_publish,
        &invalid_publish_response);

    if (publish_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT) {
        server.shutdown();
        return fail("invalid PublishMetric did not return INVALID_ARGUMENT");
    }

    api::GetCurrentRequest missing_request;
    missing_request.mutable_metric_id()->set_value("AHU01.Unknown");

    api::GetCurrentResponse missing_response;
    grpc::ClientContext missing_context;

    const auto missing_status = stub->GetCurrent(
        &missing_context,
        missing_request,
        &missing_response);

    if (missing_status.error_code() != grpc::StatusCode::NOT_FOUND) {
        server.shutdown();
        return fail("unknown GetCurrent did not return NOT_FOUND");
    }

    api::SubscribeRequest empty_subscription;
    grpc::ClientContext subscribe_context;
    auto stream = stub->Subscribe(
        &subscribe_context,
        empty_subscription);

    api::MetricUpdate update;

    if (stream->Read(&update)) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("empty subscription unexpectedly returned data");
    }

    const auto subscribe_status = stream->Finish();

    if (subscribe_status.error_code() !=
        grpc::StatusCode::INVALID_ARGUMENT) {
        server.shutdown();
        return fail("empty Subscribe did not return INVALID_ARGUMENT");
    }

    server.shutdown();
    return 0;
}

int test_client_reconnect_and_subscription_cleanup() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view metric = "AHU01.Reconnect";

    DataHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("reconnect test server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    {
        auto first_channel =
            grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

        if (!wait_for_channel(first_channel)) {
            server.shutdown();
            return fail("first client failed to connect");
        }

        auto first_stub = make_stub(first_channel);

        if (publish_double(*first_stub, metric, 31.0, 1000) != 0) {
            server.shutdown();
            return fail("reconnect test publication failed");
        }

        api::SubscribeRequest request;
        request.add_metric_ids()->set_value(std::string(metric));

        grpc::ClientContext context;
        context.set_deadline(
            std::chrono::system_clock::now() + std::chrono::seconds(5));

        auto stream = first_stub->Subscribe(&context, request);

        api::MetricUpdate retained;

        if (!stream->Read(&retained) ||
            retained.sample().value().double_value() != 31.0) {
            context.TryCancel();
            stream->Finish();
            server.shutdown();
            return fail("first subscriber did not receive retained value");
        }

        context.TryCancel();
        const auto finish_status = stream->Finish();

        if (finish_status.error_code() != grpc::StatusCode::CANCELLED) {
            server.shutdown();
            return fail("first subscriber cancellation was not observed");
        }
    }

    // A fresh channel/stub represents a client reconnect. Runtime current
    // state must remain available inside the still-running Data Hub process.
    auto second_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(second_channel)) {
        server.shutdown();
        return fail("reconnected client failed to connect");
    }

    auto second_stub = make_stub(second_channel);

    api::GetCurrentRequest get_request;
    get_request.mutable_metric_id()->set_value(std::string(metric));

    api::GetCurrentResponse get_response;
    grpc::ClientContext get_context;

    const auto get_status = second_stub->GetCurrent(
        &get_context,
        get_request,
        &get_response);

    if (!get_status.ok() ||
        !get_response.has_sample() ||
        get_response.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        get_response.sample().value().double_value() != 31.0) {
        server.shutdown();
        return fail("reconnected client did not receive current value");
    }

    api::SubscribeRequest second_request;
    second_request.add_metric_ids()->set_value(std::string(metric));

    grpc::ClientContext second_context;
    second_context.set_deadline(
        std::chrono::system_clock::now() + std::chrono::seconds(5));

    auto second_stream = second_stub->Subscribe(
        &second_context,
        second_request);

    api::MetricUpdate second_retained;

    if (!second_stream->Read(&second_retained) ||
        second_retained.sample().value().double_value() != 31.0) {
        second_context.TryCancel();
        second_stream->Finish();
        server.shutdown();
        return fail("reconnected subscriber did not receive retained value");
    }

    second_context.TryCancel();
    second_stream->Finish();

    server.shutdown();
    return 0;
}

int test_shutdown_cancels_active_subscription() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view metric = "AHU01.Shutdown";

    DataHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("shutdown test server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(channel)) {
        server.shutdown();
        return fail("shutdown test client failed to connect");
    }

    auto stub = make_stub(channel);

    if (publish_double(*stub, metric, 1.0, 1000) != 0) {
        server.shutdown();
        return fail("shutdown test publication failed");
    }

    api::SubscribeRequest request;
    request.add_metric_ids()->set_value(std::string(metric));

    grpc::ClientContext context;
    auto stream = stub->Subscribe(&context, request);

    api::MetricUpdate retained;

    if (!stream->Read(&retained)) {
        context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("active subscription was not established");
    }

    const auto start = std::chrono::steady_clock::now();
    server.shutdown();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    if (server.running() || server.bound_port() != 0) {
        context.TryCancel();
        stream->Finish();
        return fail("server still reports running after shutdown");
    }

    if (elapsed > std::chrono::seconds(4)) {
        context.TryCancel();
        stream->Finish();
        return fail("server shutdown exceeded the bounded grace period");
    }

    api::MetricUpdate after_shutdown;
    if (stream->Read(&after_shutdown)) {
        context.TryCancel();
        stream->Finish();
        return fail("subscription produced data after server shutdown");
    }

    const auto finish_status = stream->Finish();

    if (finish_status.ok()) {
        return fail("active subscription unexpectedly finished OK on shutdown");
    }

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

    if (const auto result = test_rpc_errors(); result != 0) {
        return result;
    }

    if (const auto result = test_client_reconnect_and_subscription_cleanup();
        result != 0) {
        return result;
    }

    if (const auto result = test_shutdown_cancels_active_subscription();
        result != 0) {
        return result;
    }

    std::cout << "Data Hub lifecycle and error-handling tests passed\n";
    return 0;
}
