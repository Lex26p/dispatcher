#include "dispatcher/data_hub/grpc_service.hpp"

namespace dispatcher::data_hub {

DataHubGrpcService::DataHubGrpcService(CurrentValueStore& current_values) noexcept
    : current_values_(current_values) {}

grpc::Status DataHubGrpcService::PublishMetric(
    grpc::ServerContext*,
    const v1::PublishMetricRequest* request,
    v1::PublishMetricResponse*) {
    if (request == nullptr || !request->has_sample()) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "publish request must contain a metric sample"};
    }

    if (!current_values_.put(request->sample())) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "metric sample must contain a non-empty metric id and a value"};
    }

    return grpc::Status::OK;
}

grpc::Status DataHubGrpcService::GetCurrent(
    grpc::ServerContext*,
    const v1::GetCurrentRequest* request,
    v1::GetCurrentResponse* response) {
    if (request == nullptr || response == nullptr ||
        !request->has_metric_id() || request->metric_id().value().empty()) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "get-current request must contain a non-empty metric id"};
    }

    const auto sample = current_values_.get(request->metric_id().value());

    if (!sample.has_value()) {
        return {
            grpc::StatusCode::NOT_FOUND,
            "current metric value is not available"};
    }

    response->mutable_sample()->CopyFrom(*sample);
    return grpc::Status::OK;
}

grpc::Status DataHubGrpcService::Subscribe(
    grpc::ServerContext*,
    const v1::SubscribeRequest*,
    grpc::ServerWriter<v1::MetricUpdate>*) {
    return {
        grpc::StatusCode::UNIMPLEMENTED,
        "subscriptions are implemented in CORE-001 / Step 5"};
}

grpc::Status DataHubGrpcService::WriteMetric(
    grpc::ServerContext*,
    const v1::WriteMetricRequest*,
    v1::WriteMetricResponse*) {
    return {
        grpc::StatusCode::UNIMPLEMENTED,
        "metric writes are implemented in CORE-001 / Step 7"};
}

}  // namespace dispatcher::data_hub
