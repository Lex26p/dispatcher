#pragma once

#include "dispatcher/data_hub/current_value_store.hpp"
#include "dispatcher/data_hub/grpc_service.hpp"
#include "dispatcher/data_hub/write_router.hpp"

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>
#include <string_view>

namespace dispatcher::data_hub {

class DataHubServer final {
public:
    explicit DataHubServer(std::string listen_address);
    ~DataHubServer();

    DataHubServer(const DataHubServer&) = delete;
    DataHubServer& operator=(const DataHubServer&) = delete;
    DataHubServer(DataHubServer&&) = delete;
    DataHubServer& operator=(DataHubServer&&) = delete;

    bool register_write_provider(
        std::string metric_id,
        std::shared_ptr<MetricWriteProvider> provider);

    [[nodiscard]] bool start();
    void wait();
    void shutdown();

    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] int bound_port() const noexcept;
    [[nodiscard]] std::string_view listen_address() const noexcept;

private:
    std::string listen_address_;
    CurrentValueStore current_values_;
    WriteRouter write_router_;
    DataHubGrpcService grpc_service_;
    std::unique_ptr<grpc::Server> server_;
    int bound_port_{0};
};

}  // namespace dispatcher::data_hub
