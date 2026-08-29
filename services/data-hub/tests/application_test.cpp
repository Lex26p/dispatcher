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

int test_application_metadata() {
    using dispatcher::data_hub::Application;

    if (Application::service_name() != "data-hub") {
        return fail("unexpected service name");
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

    return 0;
}

int test_current_value_store() {
    namespace api = dispatcher::data_hub::v1;
    using dispatcher::data_hub::CurrentValueStore;

    CurrentValueStore store;

    if (store.size() != 0) {
        return fail("new current value store is not empty");
    }

    api::MetricSample first;
    first.mutable_metric_id()->set_value("AHU01.Temperature");
    first.mutable_value()->set_double_value(22.1);
    first.set_source_timestamp_unix_ms(1000);

    if (!store.put(first)) {
        return fail("first current value was rejected");
    }

    api::MetricSample replacement;
    replacement.mutable_metric_id()->set_value("AHU01.Temperature");
    replacement.mutable_value()->set_double_value(22.8);
    replacement.set_source_timestamp_unix_ms(2000);

    if (!store.put(replacement)) {
        return fail("replacement current value was rejected");
    }

    const auto stored = store.get("AHU01.Temperature");

    if (!stored.has_value() ||
        stored->value().kind_case() != api::MetricValue::kDoubleValue ||
        stored->value().double_value() != 22.8 ||
        stored->source_timestamp_unix_ms() != 2000) {
        return fail("replacement did not become the current value");
    }

    if (store.size() != 1) {
        return fail("replacing a metric unexpectedly increased store size");
    }

    return 0;
}

int test_publish_and_get_over_grpc() {
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
    auto reader_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    const auto deadline =
        std::chrono::system_clock::now() + std::chrono::seconds(5);

    if (!publisher_channel->WaitForConnected(deadline) ||
        !reader_channel->WaitForConnected(deadline)) {
        server.shutdown();
        return fail("gRPC clients failed to connect");
    }

    auto publisher = api::DataHub::NewStub(publisher_channel);
    auto reader = api::DataHub::NewStub(reader_channel);

    api::PublishMetricRequest publish_request;
    publish_request.mutable_sample()->mutable_metric_id()->set_value(
        "AHU01.Temperature");
    publish_request.mutable_sample()->mutable_value()->set_double_value(23.0);
    publish_request.mutable_sample()->set_source_timestamp_unix_ms(1234);

    api::PublishMetricResponse publish_response;
    grpc::ClientContext publish_context;

    const auto publish_status = publisher->PublishMetric(
        &publish_context,
        publish_request,
        &publish_response);

    if (!publish_status.ok()) {
        server.shutdown();
        return fail("PublishMetric RPC failed");
    }

    api::GetCurrentRequest get_request;
    get_request.mutable_metric_id()->set_value("AHU01.Temperature");

    api::GetCurrentResponse get_response;
    grpc::ClientContext get_context;

    const auto get_status = reader->GetCurrent(
        &get_context,
        get_request,
        &get_response);

    if (!get_status.ok()) {
        server.shutdown();
        return fail("GetCurrent RPC failed");
    }

    if (!get_response.has_sample() ||
        get_response.sample().metric_id().value() != "AHU01.Temperature" ||
        get_response.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        get_response.sample().value().double_value() != 23.0 ||
        get_response.sample().source_timestamp_unix_ms() != 1234) {
        server.shutdown();
        return fail("GetCurrent returned an unexpected current value");
    }

    api::GetCurrentRequest missing_request;
    missing_request.mutable_metric_id()->set_value("AHU01.Unknown");

    api::GetCurrentResponse missing_response;
    grpc::ClientContext missing_context;

    const auto missing_status = reader->GetCurrent(
        &missing_context,
        missing_request,
        &missing_response);

    if (missing_status.error_code() != grpc::StatusCode::NOT_FOUND) {
        server.shutdown();
        return fail("unknown metric did not return NOT_FOUND");
    }

    api::PublishMetricRequest invalid_publish;
    invalid_publish.mutable_sample()->mutable_metric_id()->set_value(
        "AHU01.Invalid");

    api::PublishMetricResponse invalid_response;
    grpc::ClientContext invalid_context;

    const auto invalid_status = publisher->PublishMetric(
        &invalid_context,
        invalid_publish,
        &invalid_response);

    if (invalid_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT) {
        server.shutdown();
        return fail("invalid sample did not return INVALID_ARGUMENT");
    }

    server.shutdown();
    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_application_metadata(); result != 0) {
        return result;
    }

    if (const auto result = test_contract(); result != 0) {
        return result;
    }

    if (const auto result = test_current_value_store(); result != 0) {
        return result;
    }

    if (const auto result = test_publish_and_get_over_grpc(); result != 0) {
        return result;
    }

    std::cout
        << "Data Hub application, store and gRPC publish/get tests passed\n";
    return 0;
}
