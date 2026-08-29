#include "dispatcher/data_hub/application.hpp"
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

    // Referencing the generated gRPC service type ensures that both protobuf
    // and gRPC code generation are part of this test target.
    [[maybe_unused]] api::DataHub::Service* grpc_service_type = nullptr;

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

    std::cout << "Data Hub application and contract tests passed\n";
    return 0;
}
