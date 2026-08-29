#include "dispatcher/data_hub/server.hpp"

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

#include <chrono>
#include <utility>

namespace dispatcher::data_hub {

DataHubServer::DataHubServer(std::string listen_address)
    : listen_address_(std::move(listen_address)),
      grpc_service_(current_values_, write_router_) {}

DataHubServer::~DataHubServer() {
    shutdown();
}

bool DataHubServer::register_write_provider(
    std::string metric_id,
    std::shared_ptr<MetricWriteProvider> provider) {
    return write_router_.register_provider(
        std::move(metric_id),
        std::move(provider));
}

bool DataHubServer::start() {
    if (server_ != nullptr || listen_address_.empty()) {
        return false;
    }

    grpc::ServerBuilder builder;
    builder.AddListeningPort(
        listen_address_,
        grpc::InsecureServerCredentials(),
        &bound_port_);
    builder.RegisterService(&grpc_service_);

    server_ = builder.BuildAndStart();

    if (server_ == nullptr || bound_port_ <= 0) {
        server_.reset();
        bound_port_ = 0;
        return false;
    }

    return true;
}

void DataHubServer::wait() {
    if (server_ != nullptr) {
        server_->Wait();
    }
}

void DataHubServer::shutdown() {
    if (server_ == nullptr) {
        return;
    }

    // Stop accepting new calls immediately, allow active RPCs a short grace
    // period, then force cancellation so a long-lived subscription cannot
    // keep the process alive indefinitely.
    constexpr auto graceful_period = std::chrono::seconds(2);
    const auto deadline =
        std::chrono::system_clock::now() + graceful_period;

    server_->Shutdown(deadline);
    server_->Wait();
    server_.reset();
    bound_port_ = 0;
}

bool DataHubServer::running() const noexcept {
    return server_ != nullptr;
}

int DataHubServer::bound_port() const noexcept {
    return bound_port_;
}

std::string_view DataHubServer::listen_address() const noexcept {
    return listen_address_;
}

}  // namespace dispatcher::data_hub
