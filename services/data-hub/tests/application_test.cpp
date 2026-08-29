#include "dispatcher/data_hub/application.hpp"
#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/server.hpp"
#include "dispatcher/data_hub/v1/data_hub.grpc.pb.h"
#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

std::unique_ptr<dispatcher::data_hub::v1::DataHub::Stub> make_stub(
    const std::string& target) {
    return dispatcher::data_hub::v1::DataHub::NewStub(
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));
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

int test_publish_get_and_subscribe_over_grpc() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::DataHubServer;

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
            "AHU01.Temperature",
            25.0,
            1000) != 0) {
        server.shutdown();
        return fail("initial PublishMetric RPC failed");
    }

    // GetCurrent still works after subscription support is added.
    api::GetCurrentRequest get_request;
    get_request.mutable_metric_id()->set_value("AHU01.Temperature");

    api::GetCurrentResponse get_response;
    grpc::ClientContext get_context;

    const auto get_status = reader->GetCurrent(
        &get_context,
        get_request,
        &get_response);

    if (!get_status.ok() ||
        !get_response.has_sample() ||
        get_response.sample().value().double_value() != 25.0) {
        server.shutdown();
        return fail("GetCurrent failed before subscription");
    }

    api::SubscribeRequest subscribe_request;
    subscribe_request.add_metric_ids()->set_value("AHU01.Temperature");

    grpc::ClientContext subscribe_context;
    subscribe_context.set_deadline(
        std::chrono::system_clock::now() + std::chrono::seconds(10));

    auto stream = subscriber->Subscribe(
        &subscribe_context,
        subscribe_request);

    api::MetricUpdate retained;

    if (!stream->Read(&retained)) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive retained current value");
    }

    if (!retained.has_sample() ||
        retained.sample().metric_id().value() != "AHU01.Temperature" ||
        retained.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        retained.sample().value().double_value() != 25.0 ||
        retained.sample().source_timestamp_unix_ms() != 1000) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("retained subscription value is invalid");
    }

    // Publish an unrelated metric first. The next item read from this
    // subscription must still be the next Temperature update.
    if (publish_double(
            *publisher,
            "AHU01.Pressure",
            3.0,
            1500) != 0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("unrelated metric publication failed");
    }

    if (publish_double(
            *publisher,
            "AHU01.Temperature",
            26.0,
            2000) != 0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live metric publication failed");
    }

    api::MetricUpdate live;

    if (!stream->Read(&live)) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive live update");
    }

    if (!live.has_sample() ||
        live.sample().metric_id().value() != "AHU01.Temperature" ||
        live.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        live.sample().value().double_value() != 26.0 ||
        live.sample().source_timestamp_unix_ms() != 2000) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("live subscription update is invalid");
    }

    subscribe_context.TryCancel();
    const auto finish_status = stream->Finish();

    if (finish_status.error_code() != grpc::StatusCode::CANCELLED) {
        server.shutdown();
        return fail("cancelled subscription did not finish as CANCELLED");
    }

    // Empty subscriptions are deliberately invalid in the v1 contract.
    api::SubscribeRequest empty_request;
    grpc::ClientContext empty_context;
    auto empty_stream = subscriber->Subscribe(
        &empty_context,
        empty_request);

    api::MetricUpdate empty_update;
    if (empty_stream->Read(&empty_update)) {
        empty_context.TryCancel();
        empty_stream->Finish();
        server.shutdown();
        return fail("empty subscription unexpectedly produced an update");
    }

    const auto empty_status = empty_stream->Finish();

    if (empty_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT) {
        server.shutdown();
        return fail("empty subscription did not return INVALID_ARGUMENT");
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

    if (const auto result = test_publish_get_and_subscribe_over_grpc();
        result != 0) {
        return result;
    }

    std::cout
        << "Data Hub publish/get/subscription tests passed\n";
    return 0;
}
