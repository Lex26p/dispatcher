#pragma once

#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/v1/data_hub.grpc.pb.h"

#include <grpcpp/grpcpp.h>

namespace dispatcher::data_hub {

class DataHubGrpcService final : public v1::DataHub::Service {
public:
    explicit DataHubGrpcService(CurrentValueStore& current_values) noexcept;

    grpc::Status PublishMetric(
        grpc::ServerContext* context,
        const v1::PublishMetricRequest* request,
        v1::PublishMetricResponse* response) override;

    grpc::Status GetCurrent(
        grpc::ServerContext* context,
        const v1::GetCurrentRequest* request,
        v1::GetCurrentResponse* response) override;

    grpc::Status Subscribe(
        grpc::ServerContext* context,
        const v1::SubscribeRequest* request,
        grpc::ServerWriter<v1::MetricUpdate>* writer) override;

    grpc::Status WriteMetric(
        grpc::ServerContext* context,
        const v1::WriteMetricRequest* request,
        v1::WriteMetricResponse* response) override;

private:
    CurrentValueStore& current_values_;
};

}  // namespace dispatcher::data_hub
