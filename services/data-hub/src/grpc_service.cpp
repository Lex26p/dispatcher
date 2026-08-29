#include "dispatcher/data_hub/grpc_service.hpp"

#include <chrono>
#include <string>
#include <unordered_set>
#include <vector>

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

    std::lock_guard lock(publish_subscription_mutex_);

    if (!current_values_.put(request->sample())) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "metric sample must contain a non-empty metric id and a value"};
    }

    subscriptions_.publish(request->sample());
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
    grpc::ServerContext* context,
    const v1::SubscribeRequest* request,
    grpc::ServerWriter<v1::MetricUpdate>* writer) {
    if (context == nullptr || request == nullptr || writer == nullptr) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "subscription request is incomplete"};
    }

    if (request->metric_ids().empty()) {
        return {
            grpc::StatusCode::INVALID_ARGUMENT,
            "subscription must contain at least one metric id"};
    }

    std::vector<std::string> metric_ids;
    metric_ids.reserve(
        static_cast<std::size_t>(request->metric_ids_size()));

    std::unordered_set<std::string> unique_ids;

    for (const auto& metric_id : request->metric_ids()) {
        if (metric_id.value().empty()) {
            return {
                grpc::StatusCode::INVALID_ARGUMENT,
                "subscription metric ids must not be empty"};
        }

        if (unique_ids.insert(metric_id.value()).second) {
            metric_ids.push_back(metric_id.value());
        }
    }

    SubscriptionManager::Handle handle;

    {
        std::lock_guard lock(publish_subscription_mutex_);
        handle = subscriptions_.create(metric_ids);

        // Queue retained/current values in the same order as the request.
        // Publication cannot interleave with this block, so every later live
        // update is queued after these retained values.
        for (const auto& metric_id : metric_ids) {
            const auto current = current_values_.get(metric_id);

            if (current.has_value()) {
                handle.subscription->push(*current);
            }
        }
    }

    constexpr auto cancellation_poll = std::chrono::milliseconds(100);
    grpc::Status result = grpc::Status::OK;

    while (!context->IsCancelled()) {
        v1::MetricSample sample;
        const auto wait_result =
            handle.subscription->wait_next(sample, cancellation_poll);

        if (wait_result == Subscription::WaitResult::timeout) {
            continue;
        }

        if (wait_result == Subscription::WaitResult::closed) {
            break;
        }

        v1::MetricUpdate update;
        update.mutable_sample()->CopyFrom(sample);

        if (!writer->Write(update)) {
            break;
        }
    }

    subscriptions_.remove(handle.id);

    if (context->IsCancelled()) {
        result = {
            grpc::StatusCode::CANCELLED,
            "subscription cancelled"};
    }

    return result;
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
