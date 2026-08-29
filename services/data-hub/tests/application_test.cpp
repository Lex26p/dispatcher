#include "dispatcher/data_hub/application.hpp"
#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/server.hpp"
#include "dispatcher/data_hub/v1/data_hub.grpc.pb.h"
#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

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

int test_working_and_state_metrics_over_grpc() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view working_metric = "AHU01.Temperature";
    constexpr std::string_view state_metric = "AHU01.Temperature.State";

    DataHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("gRPC server failed to start");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto publisher_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto subscriber_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto reader_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(publisher_channel) ||
        !wait_for_channel(subscriber_channel) ||
        !wait_for_channel(reader_channel)) {
        server.shutdown();
        return fail("gRPC clients failed to connect");
    }

    auto publisher = api::DataHub::NewStub(publisher_channel);
    auto subscriber = api::DataHub::NewStub(subscriber_channel);
    auto reader = api::DataHub::NewStub(reader_channel);

    if (publish_double(
            *publisher,
            working_metric,
            25.0,
            1000) != 0) {
        server.shutdown();
        return fail("working metric publication failed");
    }

    if (publish_string(
            *publisher,
            state_metric,
            "Warning",
            1100) != 0) {
        server.shutdown();
        return fail("state metric publication failed");
    }

    api::GetCurrentRequest working_get;
    working_get.mutable_metric_id()->set_value(std::string(working_metric));

    api::GetCurrentResponse working_response;
    grpc::ClientContext working_context;

    const auto working_status = reader->GetCurrent(
        &working_context,
        working_get,
        &working_response);

    if (!working_status.ok() ||
        !working_response.has_sample() ||
        working_response.sample().metric_id().value() != working_metric ||
        working_response.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        working_response.sample().value().double_value() != 25.0) {
        server.shutdown();
        return fail("working metric GetCurrent result is invalid");
    }

    api::GetCurrentRequest state_get;
    state_get.mutable_metric_id()->set_value(std::string(state_metric));

    api::GetCurrentResponse state_response;
    grpc::ClientContext state_context;

    const auto state_status = reader->GetCurrent(
        &state_context,
        state_get,
        &state_response);

    if (!state_status.ok() ||
        !state_response.has_sample() ||
        state_response.sample().metric_id().value() != state_metric ||
        state_response.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        state_response.sample().value().string_value() != "Warning") {
        server.shutdown();
        return fail("state metric GetCurrent result is invalid");
    }

    api::SubscribeRequest subscribe_request;
    subscribe_request.add_metric_ids()->set_value(std::string(working_metric));
    subscribe_request.add_metric_ids()->set_value(std::string(state_metric));

    grpc::ClientContext subscribe_context;
    subscribe_context.set_deadline(
        std::chrono::system_clock::now() + std::chrono::seconds(10));

    auto stream = subscriber->Subscribe(
        &subscribe_context,
        subscribe_request);

    api::MetricUpdate retained_working;
    api::MetricUpdate retained_state;

    if (!stream->Read(&retained_working) ||
        !stream->Read(&retained_state)) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive retained metric pair");
    }

    if (!retained_working.has_sample() ||
        retained_working.sample().metric_id().value() != working_metric ||
        retained_working.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        retained_working.sample().value().double_value() != 25.0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("retained working metric is invalid");
    }

    if (!retained_state.has_sample() ||
        retained_state.sample().metric_id().value() != state_metric ||
        retained_state.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        retained_state.sample().value().string_value() != "Warning") {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("retained state metric is invalid");
    }

    if (publish_double(
            *publisher,
            working_metric,
            26.0,
            2000) != 0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live working metric publication failed");
    }

    if (publish_string(
            *publisher,
            state_metric,
            "Alarm",
            2100) != 0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live state metric publication failed");
    }

    api::MetricUpdate live_working;
    api::MetricUpdate live_state;

    if (!stream->Read(&live_working) ||
        !stream->Read(&live_state)) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive live metric pair");
    }

    if (!live_working.has_sample() ||
        live_working.sample().metric_id().value() != working_metric ||
        live_working.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        live_working.sample().value().double_value() != 26.0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live working metric is invalid");
    }

    if (!live_state.has_sample() ||
        live_state.sample().metric_id().value() != state_metric ||
        live_state.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        live_state.sample().value().string_value() != "Alarm") {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live state metric is invalid");
    }

    subscribe_context.TryCancel();
    const auto finish_status = stream->Finish();

    if (finish_status.error_code() != grpc::StatusCode::CANCELLED) {
        server.shutdown();
        return fail("cancelled subscription did not finish as CANCELLED");
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

    if (const auto result = test_working_and_state_metrics_over_grpc();
        result != 0) {
        return result;
    }

    std::cout
        << "Data Hub working/state metric tests passed\n";
    return 0;
}
