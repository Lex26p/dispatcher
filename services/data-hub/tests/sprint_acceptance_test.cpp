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
#include <optional>
#include <string>
#include <string_view>

namespace {

namespace api = dispatcher::data_hub::v1;

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

bool wait_for_channel(const std::shared_ptr<grpc::Channel>& channel) {
    return channel->WaitForConnected(
        std::chrono::system_clock::now() + std::chrono::seconds(5));
}

class RecordingWriteProvider final
    : public dispatcher::data_hub::MetricWriteProvider {
public:
    bool write(const api::WriteMetricRequest& request) override {
        std::lock_guard lock(mutex_);
        last_request_ = request;
        return true;
    }

    [[nodiscard]] std::optional<api::WriteMetricRequest> last_request() const {
        std::lock_guard lock(mutex_);
        return last_request_;
    }

private:
    mutable std::mutex mutex_;
    std::optional<api::WriteMetricRequest> last_request_;
};

grpc::Status publish_double(
    api::DataHub::Stub& stub,
    std::string_view metric_id,
    const double value,
    const std::int64_t source_timestamp_unix_ms) {
    api::PublishMetricRequest request;
    request.mutable_sample()->mutable_metric_id()->set_value(
        std::string(metric_id));
    request.mutable_sample()->mutable_value()->set_double_value(value);
    request.mutable_sample()->set_source_timestamp_unix_ms(
        source_timestamp_unix_ms);

    api::PublishMetricResponse response;
    grpc::ClientContext context;

    return stub.PublishMetric(&context, request, &response);
}

grpc::Status publish_string(
    api::DataHub::Stub& stub,
    std::string_view metric_id,
    std::string_view value,
    const std::int64_t source_timestamp_unix_ms) {
    api::PublishMetricRequest request;
    request.mutable_sample()->mutable_metric_id()->set_value(
        std::string(metric_id));
    request.mutable_sample()->mutable_value()->set_string_value(
        std::string(value));
    request.mutable_sample()->set_source_timestamp_unix_ms(
        source_timestamp_unix_ms);

    api::PublishMetricResponse response;
    grpc::ClientContext context;

    return stub.PublishMetric(&context, request, &response);
}

int run_acceptance_scenario() {
    using dispatcher::data_hub::DataHubServer;

    constexpr std::string_view temperature = "AHU01.Temperature";
    constexpr std::string_view temperature_state =
        "AHU01.Temperature.State";
    constexpr std::string_view setpoint = "AHU01.Setpoint";

    auto write_provider = std::make_shared<RecordingWriteProvider>();

    DataHubServer server("127.0.0.1:0");

    if (!server.register_write_provider(
            std::string(setpoint),
            write_provider)) {
        return fail("failed to register test write provider");
    }

    if (!server.start()) {
        return fail("failed to start Data Hub");
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(server.bound_port());

    auto publisher_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto reader_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto subscriber_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto writer_channel =
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    if (!wait_for_channel(publisher_channel) ||
        !wait_for_channel(reader_channel) ||
        !wait_for_channel(subscriber_channel) ||
        !wait_for_channel(writer_channel)) {
        server.shutdown();
        return fail("one or more acceptance clients failed to connect");
    }

    auto publisher = api::DataHub::NewStub(publisher_channel);
    auto reader = api::DataHub::NewStub(reader_channel);
    auto subscriber = api::DataHub::NewStub(subscriber_channel);
    auto writer = api::DataHub::NewStub(writer_channel);

    // 1. Publisher -> Temperature = 23.
    const auto first_publish =
        publish_double(*publisher, temperature, 23.0, 1000);

    if (!first_publish.ok()) {
        server.shutdown();
        return fail("Temperature=23 publication failed");
    }

    // 2-3. Another client reads the current value and gets 23.
    api::GetCurrentRequest get_temperature_request;
    get_temperature_request.mutable_metric_id()->set_value(
        std::string(temperature));

    api::GetCurrentResponse get_temperature_response;
    grpc::ClientContext get_temperature_context;

    const auto get_temperature_status = reader->GetCurrent(
        &get_temperature_context,
        get_temperature_request,
        &get_temperature_response);

    if (!get_temperature_status.ok() ||
        !get_temperature_response.has_sample() ||
        get_temperature_response.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        get_temperature_response.sample().value().double_value() != 23.0) {
        server.shutdown();
        return fail("GetCurrent did not return Temperature=23");
    }

    // Subscribe to both the working metric and its state. The state does not
    // exist yet, so the first retained item must be only Temperature=23.
    api::SubscribeRequest subscribe_request;
    subscribe_request.add_metric_ids()->set_value(std::string(temperature));
    subscribe_request.add_metric_ids()->set_value(
        std::string(temperature_state));

    grpc::ClientContext subscribe_context;
    subscribe_context.set_deadline(
        std::chrono::system_clock::now() + std::chrono::seconds(10));

    auto stream = subscriber->Subscribe(
        &subscribe_context,
        subscribe_request);

    api::MetricUpdate retained_temperature;

    if (!stream->Read(&retained_temperature) ||
        !retained_temperature.has_sample() ||
        retained_temperature.sample().metric_id().value() != temperature ||
        retained_temperature.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        retained_temperature.sample().value().double_value() != 23.0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive retained Temperature=23");
    }

    // 5-6. Publish Temperature=24 and observe the live update.
    const auto second_publish =
        publish_double(*publisher, temperature, 24.0, 2000);

    if (!second_publish.ok()) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("Temperature=24 publication failed");
    }

    api::MetricUpdate live_temperature;

    if (!stream->Read(&live_temperature) ||
        !live_temperature.has_sample() ||
        live_temperature.sample().metric_id().value() != temperature ||
        live_temperature.sample().value().kind_case() !=
            api::MetricValue::kDoubleValue ||
        live_temperature.sample().value().double_value() != 24.0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive live Temperature=24");
    }

    // 7-8. Publish the state metric through the same generic path.
    const auto state_publish =
        publish_string(*publisher, temperature_state, "Warning", 2100);

    if (!state_publish.ok()) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("Temperature.State=Warning publication failed");
    }

    api::MetricUpdate live_state;

    if (!stream->Read(&live_state) ||
        !live_state.has_sample() ||
        live_state.sample().metric_id().value() != temperature_state ||
        live_state.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        live_state.sample().value().string_value() != "Warning") {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("subscriber did not receive Temperature.State=Warning");
    }

    api::GetCurrentRequest get_state_request;
    get_state_request.mutable_metric_id()->set_value(
        std::string(temperature_state));

    api::GetCurrentResponse get_state_response;
    grpc::ClientContext get_state_context;

    const auto get_state_status = reader->GetCurrent(
        &get_state_context,
        get_state_request,
        &get_state_response);

    if (!get_state_status.ok() ||
        !get_state_response.has_sample() ||
        get_state_response.sample().value().kind_case() !=
            api::MetricValue::kStringValue ||
        get_state_response.sample().value().string_value() != "Warning") {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("GetCurrent did not return Temperature.State=Warning");
    }

    // 9-10. Write Setpoint=25 and verify delivery to the test provider.
    api::WriteMetricRequest write_request;
    write_request.mutable_metric_id()->set_value(std::string(setpoint));
    write_request.mutable_value()->set_double_value(25.0);

    api::WriteMetricResponse write_response;
    grpc::ClientContext write_context;

    const auto write_status = writer->WriteMetric(
        &write_context,
        write_request,
        &write_response);

    if (!write_status.ok()) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("WriteMetric(Setpoint,25) failed");
    }

    const auto delivered = write_provider->last_request();

    if (!delivered.has_value() ||
        !delivered->has_metric_id() ||
        delivered->metric_id().value() != setpoint ||
        !delivered->has_value() ||
        delivered->value().kind_case() != api::MetricValue::kDoubleValue ||
        delivered->value().double_value() != 25.0) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("test write provider did not receive Setpoint=25");
    }

    // A write request alone must not create a factual current value.
    api::GetCurrentRequest get_setpoint_request;
    get_setpoint_request.mutable_metric_id()->set_value(
        std::string(setpoint));

    api::GetCurrentResponse get_setpoint_response;
    grpc::ClientContext get_setpoint_context;

    const auto get_setpoint_status = reader->GetCurrent(
        &get_setpoint_context,
        get_setpoint_request,
        &get_setpoint_response);

    if (get_setpoint_status.error_code() != grpc::StatusCode::NOT_FOUND) {
        subscribe_context.TryCancel();
        stream->Finish();
        server.shutdown();
        return fail("WriteMetric incorrectly created a current Setpoint");
    }

    subscribe_context.TryCancel();
    const auto subscribe_finish = stream->Finish();

    if (subscribe_finish.error_code() != grpc::StatusCode::CANCELLED) {
        server.shutdown();
        return fail("acceptance subscription did not cancel cleanly");
    }

    server.shutdown();
    return 0;
}

}  // namespace

int main() {
    const auto result = run_acceptance_scenario();

    if (result != 0) {
        return result;
    }

    std::cout << "CORE-001 Data Hub acceptance scenario passed\n";
    return 0;
}
