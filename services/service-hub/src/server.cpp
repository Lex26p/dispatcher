#include "dispatcher/service_hub/server.hpp"

#include "dispatcher/service_hub/provider_registry.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <poll.h>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dispatcher::service_hub {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::string_view kEndpoint = "/v1/ws";
constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::size_t kMaximumMessageSize = 1024 * 1024;
constexpr int kDefaultTimeoutMs = 5000;
constexpr int kMaximumTimeoutMs = 60000;

using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

[[nodiscard]] JsonPtr adopt_json(json_object* value) {
    return JsonPtr(value, &json_object_put);
}

[[nodiscard]] std::optional<JsonPtr> parse_json(const std::string_view text) {
    auto* tokener = json_tokener_new();
    if (tokener == nullptr) {
        return std::nullopt;
    }

    json_object* value = json_tokener_parse_ex(
        tokener,
        text.data(),
        static_cast<int>(text.size()));

    const auto error = json_tokener_get_error(tokener);
    const auto parse_end = json_tokener_get_parse_end(tokener);
    json_tokener_free(tokener);

    if (error != json_tokener_success ||
        value == nullptr ||
        parse_end != text.size()) {
        if (value != nullptr) {
            json_object_put(value);
        }
        return std::nullopt;
    }

    return adopt_json(value);
}

[[nodiscard]] std::string serialize_json(json_object* value) {
    return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
}

[[nodiscard]] bool get_string(
    json_object* object,
    const char* name,
    std::string& result) {
    json_object* value = nullptr;

    if (!json_object_object_get_ex(object, name, &value) ||
        !json_object_is_type(value, json_type_string)) {
        return false;
    }

    result = json_object_get_string(value);
    return true;
}

[[nodiscard]] bool get_bool(
    json_object* object,
    const char* name,
    bool& result) {
    json_object* value = nullptr;

    if (!json_object_object_get_ex(object, name, &value) ||
        !json_object_is_type(value, json_type_boolean)) {
        return false;
    }

    result = json_object_get_boolean(value) != 0;
    return true;
}

[[nodiscard]] bool is_valid_request_id(
    const std::string_view value) noexcept {
    if (value.empty() || value.size() > 128) {
        return false;
    }

    const auto first = value.front();
    const bool first_valid =
        (first >= 'a' && first <= 'z') ||
        (first >= 'A' && first <= 'Z') ||
        (first >= '0' && first <= '9');

    if (!first_valid) {
        return false;
    }

    for (const char character : value) {
        const bool valid =
            (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '.' ||
            character == '_' ||
            character == ':' ||
            character == '-';

        if (!valid) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool header_has_subprotocol(
    const http::request<http::string_body>& request) {
    const auto value = request[http::field::sec_websocket_protocol];

    std::string_view text(value.data(), value.size());

    while (!text.empty()) {
        const auto comma = text.find(',');
        auto token = text.substr(0, comma);

        while (!token.empty() &&
               (token.front() == ' ' || token.front() == '\t')) {
            token.remove_prefix(1);
        }

        while (!token.empty() &&
               (token.back() == ' ' || token.back() == '\t')) {
            token.remove_suffix(1);
        }

        if (token == kSubprotocol) {
            return true;
        }

        if (comma == std::string_view::npos) {
            break;
        }

        text.remove_prefix(comma + 1);
    }

    return false;
}

[[nodiscard]] std::optional<std::pair<std::string, unsigned short>>
parse_listen_address(const std::string_view address) {
    const auto separator = address.rfind(':');

    if (separator == std::string_view::npos ||
        separator == 0 ||
        separator + 1 >= address.size()) {
        return std::nullopt;
    }

    std::string host(address.substr(0, separator));
    const std::string port_text(address.substr(separator + 1));

    unsigned long port_value = 0;

    try {
        std::size_t parsed = 0;
        port_value = std::stoul(port_text, &parsed);

        if (parsed != port_text.size() || port_value > 65535) {
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::make_pair(std::move(host),
                          static_cast<unsigned short>(port_value));
}

[[nodiscard]] JsonPtr make_error(
    const std::string_view code,
    const std::string_view message) {
    auto error = adopt_json(json_object_new_object());
    json_object_object_add(
        error.get(),
        "code",
        json_object_new_string_len(
            code.data(),
            static_cast<int>(code.size())));
    json_object_object_add(
        error.get(),
        "message",
        json_object_new_string_len(
            message.data(),
            static_cast<int>(message.size())));
    return error;
}

[[nodiscard]] std::string make_protocol_error(
    const std::string_view code,
    const std::string_view message) {
    auto root = adopt_json(json_object_new_object());
    json_object_object_add(root.get(), "type", json_object_new_string("protocol_error"));

    auto error = make_error(code, message);
    json_object_object_add(root.get(), "error", error.release());
    return serialize_json(root.get());
}

[[nodiscard]] std::string make_request_error(
    const std::string_view id,
    const std::string_view code,
    const std::string_view message) {
    auto root = adopt_json(json_object_new_object());
    json_object_object_add(root.get(), "type", json_object_new_string("response"));
    json_object_object_add(
        root.get(),
        "id",
        json_object_new_string_len(
            id.data(),
            static_cast<int>(id.size())));
    json_object_object_add(root.get(), "ok", json_object_new_boolean(false));

    auto error = make_error(code, message);
    json_object_object_add(root.get(), "error", error.release());
    return serialize_json(root.get());
}

}  // namespace

class ServiceHubServer::Impl final {
public:
    class Session;

    struct PendingRequest final {
        std::weak_ptr<Session> client_session;
        std::string client_request_id;
        std::chrono::steady_clock::time_point deadline;
        bool timeout_armed{false};
    };

    class Session final : public std::enable_shared_from_this<Session> {
    public:
        Session(
            Impl& owner,
            const ProviderConnectionId connection_id,
            tcp::socket socket)
            : owner_(owner),
              connection_id_(connection_id),
              socket_(std::move(socket)) {}

        void run() {
            if (!perform_handshake()) {
                owner_.on_session_closed(connection_id_);
                return;
            }

            std::string first_message;

            if (!read_text(first_message)) {
                owner_.on_session_closed(connection_id_);
                return;
            }

            const auto parsed = parse_json(first_message);

            if (!parsed.has_value() ||
                !json_object_is_type(parsed->get(), json_type_object)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "First application message must be a JSON object"));
                owner_.on_session_closed(connection_id_);
                return;
            }

            std::string type;

            if (!get_string(parsed->get(), "type", type)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Application message is missing type"));
                owner_.on_session_closed(connection_id_);
                return;
            }

            if (type == "register") {
                run_provider(parsed->get());
            } else if (type == "request") {
                run_client(first_message);
            } else {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Connection role could not be determined"));
            }

            owner_.on_session_closed(connection_id_);
        }

        [[nodiscard]] bool enqueue_provider_message(std::string message) {
            return enqueue_outbound_message(std::move(message));
        }

        [[nodiscard]] bool complete_client_request(
            const std::string& client_request_id,
            std::string message) {
            finish_client_request(client_request_id);
            return enqueue_outbound_message(std::move(message));
        }

        void stop() {
            active_.store(false);

            beast::error_code error;

            if (websocket_.has_value()) {
                beast::get_lowest_layer(*websocket_).cancel(error);
                error = {};
                beast::get_lowest_layer(*websocket_).close(error);
            } else {
                socket_.cancel(error);
                error = {};
                socket_.close(error);
            }
        }

    private:
        [[nodiscard]] bool perform_handshake() {
            beast::flat_buffer buffer;
            http::request<http::string_body> request;
            beast::error_code error;

            http::read(socket_, buffer, request, error);

            if (error) {
                return false;
            }

            if (!websocket::is_upgrade(request) ||
                request.target() != kEndpoint ||
                !header_has_subprotocol(request)) {
                http::response<http::string_body> response{
                    http::status::bad_request,
                    request.version()};
                response.set(http::field::content_type, "text/plain");
                response.keep_alive(false);
                response.body() = "Unsupported Service Hub WebSocket endpoint or subprotocol\n";
                response.prepare_payload();
                http::write(socket_, response, error);
                return false;
            }

            websocket_.emplace(std::move(socket_));
            websocket_->read_message_max(kMaximumMessageSize);
            websocket_->set_option(
                websocket::stream_base::decorator(
                    [](websocket::response_type& response) {
                        response.set(
                            http::field::sec_websocket_protocol,
                            kSubprotocol);
                    }));

            websocket_->accept(request, error);

            if (error) {
                return false;
            }

            active_.store(true);
            return true;
        }

        [[nodiscard]] bool read_text(std::string& message) {
            beast::flat_buffer buffer;
            beast::error_code error;

            websocket_->read(buffer, error);

            if (error) {
                return false;
            }

            if (!websocket_->got_text()) {
                return false;
            }

            message = beast::buffers_to_string(buffer.data());
            return true;
        }

        bool send_text(const std::string_view message) {
            beast::error_code error;
            websocket_->text(true);
            websocket_->write(asio::buffer(message), error);
            return !error;
        }

        [[nodiscard]] bool enqueue_outbound_message(std::string message) {
            std::lock_guard lock(outbound_mutex_);

            if (!active_.load()) {
                return false;
            }

            outbound_.push_back(std::move(message));
            return true;
        }

        [[nodiscard]] bool flush_outbound_messages() {
            std::deque<std::string> outbound;

            {
                std::lock_guard lock(outbound_mutex_);
                outbound.swap(outbound_);
            }

            for (const auto& message : outbound) {
                if (!send_text(message)) {
                    active_.store(false);
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool try_begin_client_request(
            const std::string& request_id) {
            std::lock_guard lock(active_client_requests_mutex_);
            return active_client_requests_.insert(request_id).second;
        }

        void finish_client_request(const std::string& request_id) {
            std::lock_guard lock(active_client_requests_mutex_);
            active_client_requests_.erase(request_id);
        }

        void run_provider(json_object* registration) {
            std::string service;

            if (!get_string(registration, "service", service)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider registration requires a service address"));
                return;
            }

            const auto result =
                owner_.provider_registry_.register_provider(
                    connection_id_,
                    service);

            switch (result) {
            case ProviderRegistry::RegisterResult::registered:
                break;
            case ProviderRegistry::RegisterResult::invalid_service:
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider service address is invalid"));
                return;
            case ProviderRegistry::RegisterResult::service_in_use:
                send_text(make_protocol_error(
                    "hub.service_in_use",
                    "Service already has an active provider"));
                return;
            case ProviderRegistry::RegisterResult::connection_already_registered:
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider connection is already registered"));
                return;
            }

            auto registered = adopt_json(json_object_new_object());
            json_object_object_add(
                registered.get(),
                "type",
                json_object_new_string("registered"));
            json_object_object_add(
                registered.get(),
                "service",
                json_object_new_string(service.c_str()));

            if (!send_text(serialize_json(registered.get()))) {
                return;
            }

            run_provider_loop();
        }

        void run_provider_loop() {
            while (active_.load() && owner_.running_.load()) {
                if (!flush_outbound_messages()) {
                    return;
                }

                pollfd descriptor{
                    beast::get_lowest_layer(*websocket_).native_handle(),
                    POLLIN,
                    0};

                const int poll_result = ::poll(&descriptor, 1, 10);

                if (poll_result < 0) {
                    active_.store(false);
                    return;
                }

                if (poll_result == 0) {
                    continue;
                }

                if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                    active_.store(false);
                    return;
                }

                if ((descriptor.revents & POLLIN) == 0) {
                    continue;
                }

                std::string message;

                if (!read_text(message)) {
                    active_.store(false);
                    return;
                }
                if (!handle_provider_response(message)) {
                    active_.store(false);
                    return;
                }
            }
        }

        [[nodiscard]] bool handle_provider_response(
            const std::string_view message) {
            const auto parsed = parse_json(message);

            if (!parsed.has_value() ||
                !json_object_is_type(parsed->get(), json_type_object)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider message must be a JSON object"));
                return false;
            }

            std::string type;
            std::string request_id;
            bool ok = false;

            if (!get_string(parsed->get(), "type", type) ||
                type != "response" ||
                !get_string(parsed->get(), "id", request_id) ||
                !is_valid_request_id(request_id) ||
                !get_bool(parsed->get(), "ok", ok)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider sent an invalid response"));
                return false;
            }

            json_object* payload = nullptr;
            json_object* error = nullptr;

            if (ok) {
                if (!json_object_object_get_ex(
                        parsed->get(),
                        "payload",
                        &payload)) {
                    send_text(make_protocol_error(
                        "hub.protocol_error",
                        "Successful provider response requires payload"));
                    return false;
                }
            } else {
                if (!json_object_object_get_ex(
                        parsed->get(),
                        "error",
                        &error) ||
                    !json_object_is_type(error, json_type_object)) {
                    send_text(make_protocol_error(
                        "hub.protocol_error",
                        "Failed provider response requires error"));
                    return false;
                }
            }

            if (!owner_.complete_pending_request(
                    request_id,
                    serialize_json(parsed->get()))) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Provider response refers to an unknown request"));
                return false;
            }

            return true;
        }

        void run_client(const std::string& first_message) {
            if (!handle_client_request(first_message)) {
                return;
            }

            while (active_.load() && owner_.running_.load()) {
                if (!flush_outbound_messages()) {
                    return;
                }

                pollfd descriptor{
                    beast::get_lowest_layer(*websocket_).native_handle(),
                    POLLIN,
                    0};

                const int poll_result = ::poll(&descriptor, 1, 10);

                if (poll_result < 0) {
                    active_.store(false);
                    return;
                }

                if (poll_result == 0) {
                    continue;
                }

                if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                    active_.store(false);
                    return;
                }

                if ((descriptor.revents & POLLIN) == 0) {
                    continue;
                }

                std::string message;

                if (!read_text(message)) {
                    active_.store(false);
                    return;
                }

                if (!handle_client_request(message)) {
                    active_.store(false);
                    return;
                }
            }
        }

        [[nodiscard]] bool handle_client_request(
            const std::string_view message) {
            const auto parsed = parse_json(message);

            if (!parsed.has_value() ||
                !json_object_is_type(parsed->get(), json_type_object)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Client message must be a JSON object"));
                return false;
            }

            std::string type;
            std::string client_request_id;
            std::string service;
            std::string operation;

            if (!get_string(parsed->get(), "type", type) ||
                type != "request" ||
                !get_string(parsed->get(), "id", client_request_id) ||
                !is_valid_request_id(client_request_id) ||
                !get_string(parsed->get(), "service", service) ||
                !ProviderRegistry::is_valid_service_address(service) ||
                !get_string(parsed->get(), "operation", operation) ||
                !ProviderRegistry::is_valid_service_address(operation)) {
                return send_text(make_request_error(
                    client_request_id.empty() ? "invalid" : client_request_id,
                    "hub.invalid_request",
                    "Client request envelope is invalid"));
            }

            json_object* payload = nullptr;

            if (!json_object_object_get_ex(
                    parsed->get(),
                    "payload",
                    &payload)) {
                return send_text(make_request_error(
                    client_request_id,
                    "hub.invalid_request",
                    "Client request requires payload"));
            }

            int timeout_ms = kDefaultTimeoutMs;
            json_object* timeout = nullptr;

            if (json_object_object_get_ex(
                    parsed->get(),
                    "timeout_ms",
                    &timeout)) {
                if (!json_object_is_type(timeout, json_type_int)) {
                    return send_text(make_request_error(
                        client_request_id,
                        "hub.invalid_request",
                        "timeout_ms must be an integer"));
                }

                const auto timeout_value = json_object_get_int64(timeout);

                if (timeout_value < 1 || timeout_value > kMaximumTimeoutMs) {
                    return send_text(make_request_error(
                        client_request_id,
                        "hub.invalid_request",
                        "timeout_ms is outside the supported range"));
                }

                timeout_ms = static_cast<int>(timeout_value);
            }

            if (!try_begin_client_request(client_request_id)) {
                send_text(make_protocol_error(
                    "hub.protocol_error",
                    "Client reused an active request identifier"));
                return false;
            }

            const auto fail_request =
                [this, &client_request_id](
                    const std::string_view code,
                    const std::string_view error_message) {
                    finish_client_request(client_request_id);
                    return send_text(make_request_error(
                        client_request_id,
                        code,
                        error_message));
                };

            const auto provider_id =
                owner_.provider_registry_.find_provider(service);

            if (!provider_id.has_value()) {
                return fail_request(
                    "hub.unknown_service",
                    "No active provider is registered for the requested service");
            }

            const auto provider_session =
                owner_.find_session(*provider_id);

            if (!provider_session) {
                return fail_request(
                    "hub.unknown_service",
                    "No active provider is registered for the requested service");
            }

            const std::string provider_request_id =
                owner_.next_provider_request_id();

            auto pending = std::make_shared<PendingRequest>();
            pending->client_session = shared_from_this();
            pending->client_request_id = client_request_id;
            pending->deadline =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(timeout_ms);

            if (!owner_.add_pending_request(
                    provider_request_id,
                    pending)) {
                return fail_request(
                    "hub.protocol_error",
                    "Could not allocate provider request identifier");
            }

            auto forwarded = adopt_json(json_object_new_object());
            json_object_object_add(
                forwarded.get(),
                "type",
                json_object_new_string("request"));
            json_object_object_add(
                forwarded.get(),
                "id",
                json_object_new_string(provider_request_id.c_str()));
            json_object_object_add(
                forwarded.get(),
                "service",
                json_object_new_string(service.c_str()));
            json_object_object_add(
                forwarded.get(),
                "operation",
                json_object_new_string(operation.c_str()));
            json_object_object_add(
                forwarded.get(),
                "payload",
                json_object_get(payload));
            json_object_object_add(
                forwarded.get(),
                "timeout_ms",
                json_object_new_int(timeout_ms));

            if (!provider_session->enqueue_provider_message(
                    serialize_json(forwarded.get()))) {
                owner_.remove_pending_request(provider_request_id);
                return fail_request(
                    "hub.provider_unavailable",
                    "Provider connection is unavailable");
            }

            owner_.arm_pending_request(provider_request_id);
            return true;
        }

        Impl& owner_;
        ProviderConnectionId connection_id_;
        tcp::socket socket_;
        std::optional<websocket::stream<tcp::socket>> websocket_;
        std::atomic<bool> active_{false};
        std::mutex outbound_mutex_;
        std::deque<std::string> outbound_;
        std::mutex active_client_requests_mutex_;
        std::unordered_set<std::string> active_client_requests_;
    };

    explicit Impl(std::string listen_address)
        : listen_address_(std::move(listen_address)) {}

    ~Impl() {
        shutdown();
    }

    [[nodiscard]] bool start() {
        if (running_.load()) {
            return false;
        }

        const auto parsed = parse_listen_address(listen_address_);

        if (!parsed.has_value()) {
            return false;
        }

        beast::error_code error;
        const auto address = asio::ip::make_address(parsed->first, error);

        if (error) {
            return false;
        }

        acceptor_ = std::make_unique<tcp::acceptor>(io_context_);
        const tcp::endpoint endpoint(address, parsed->second);

        acceptor_->open(endpoint.protocol(), error);

        if (error) {
            acceptor_.reset();
            return false;
        }

        acceptor_->set_option(
            asio::socket_base::reuse_address(true),
            error);

        if (error) {
            acceptor_.reset();
            return false;
        }

        acceptor_->bind(endpoint, error);

        if (error) {
            acceptor_.reset();
            return false;
        }

        acceptor_->listen(
            asio::socket_base::max_listen_connections,
            error);

        if (error) {
            acceptor_.reset();
            return false;
        }

        acceptor_->non_blocking(true, error);

        if (error) {
            acceptor_.reset();
            return false;
        }

        bound_port_ = acceptor_->local_endpoint(error).port();

        if (error || bound_port_ == 0) {
            acceptor_.reset();
            bound_port_ = 0;
            return false;
        }

        running_.store(true);
        listener_thread_ = std::thread([this] {
            accept_loop();
        });
        timeout_thread_ = std::thread([this] {
            timeout_loop();
        });

        return true;
    }

    void shutdown() {
        if (!running_.exchange(false)) {
            return;
        }

        beast::error_code error;

        if (acceptor_) {
            acceptor_->cancel(error);
            error = {};
            acceptor_->close(error);
        }

        std::vector<std::shared_ptr<Session>> sessions;

        {
            std::lock_guard lock(sessions_mutex_);

            for (const auto& [connection_id, session] : sessions_) {
                (void)connection_id;
                sessions.push_back(session);
            }
        }

        for (const auto& session : sessions) {
            session->stop();
        }

        if (listener_thread_.joinable()) {
            listener_thread_.join();
        }

        if (timeout_thread_.joinable()) {
            timeout_thread_.join();
        }

        std::vector<std::thread> threads;

        {
            std::lock_guard lock(session_threads_mutex_);
            threads.swap(session_threads_);
        }

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        {
            std::lock_guard lock(sessions_mutex_);
            sessions_.clear();
        }

        {
            std::lock_guard lock(pending_mutex_);
            pending_requests_.clear();
        }

        acceptor_.reset();
        bound_port_ = 0;
    }

    [[nodiscard]] bool running() const noexcept {
        return running_.load();
    }

    [[nodiscard]] int bound_port() const noexcept {
        return bound_port_;
    }

    [[nodiscard]] std::string_view listen_address() const noexcept {
        return listen_address_;
    }

private:
    void accept_loop() {
        while (running_.load()) {
            tcp::socket socket(io_context_);
            beast::error_code error;

            acceptor_->accept(socket, error);

            if (error) {
                if (!running_.load()) {
                    return;
                }

                if (error == asio::error::would_block ||
                    error == asio::error::try_again) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(5));
                    continue;
                }

                continue;
            }

            const auto connection_id =
                next_connection_id_.fetch_add(1);

            auto session = std::make_shared<Session>(
                *this,
                connection_id,
                std::move(socket));

            {
                std::lock_guard lock(sessions_mutex_);
                sessions_[connection_id] = session;
            }

            {
                std::lock_guard lock(session_threads_mutex_);
                session_threads_.emplace_back(
                    [session] {
                        session->run();
                    });
            }
        }
    }

    void timeout_loop() {
        while (running_.load()) {
            expire_pending_requests();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    void expire_pending_requests() {
        std::vector<std::shared_ptr<PendingRequest>> expired;
        const auto now = std::chrono::steady_clock::now();

        {
            std::lock_guard lock(pending_mutex_);

            for (auto iterator = pending_requests_.begin();
                 iterator != pending_requests_.end();) {
                const auto& pending = iterator->second;

                if (pending->timeout_armed &&
                    pending->deadline <= now) {
                    expired.push_back(pending);
                    iterator = pending_requests_.erase(iterator);
                } else {
                    ++iterator;
                }
            }
        }

        for (const auto& pending : expired) {
            if (const auto client = pending->client_session.lock()) {
                (void)client->complete_client_request(
                    pending->client_request_id,
                    make_request_error(
                        pending->client_request_id,
                        "hub.timeout",
                        "Provider did not respond before the request deadline"));
            }
        }
    }

    void on_session_closed(
        const ProviderConnectionId connection_id) {
        (void)provider_registry_.unregister_provider(connection_id);

        std::lock_guard lock(sessions_mutex_);
        sessions_.erase(connection_id);
    }

    [[nodiscard]] std::shared_ptr<Session> find_session(
        const ProviderConnectionId connection_id) {
        std::lock_guard lock(sessions_mutex_);

        const auto iterator = sessions_.find(connection_id);

        if (iterator == sessions_.end()) {
            return {};
        }

        return iterator->second;
    }

    [[nodiscard]] std::string next_provider_request_id() {
        return "hub-" +
               std::to_string(
                   next_provider_request_id_.fetch_add(1));
    }

    [[nodiscard]] bool add_pending_request(
        const std::string& request_id,
        std::shared_ptr<PendingRequest> pending) {
        std::lock_guard lock(pending_mutex_);
        return pending_requests_.emplace(
            request_id,
            std::move(pending)).second;
    }

    void arm_pending_request(const std::string& request_id) {
        std::lock_guard lock(pending_mutex_);

        const auto iterator = pending_requests_.find(request_id);

        if (iterator != pending_requests_.end()) {
            iterator->second->timeout_armed = true;
        }
    }

    void remove_pending_request(
        const std::string& request_id) {
        std::lock_guard lock(pending_mutex_);
        pending_requests_.erase(request_id);
    }

    [[nodiscard]] bool complete_pending_request(
        const std::string_view request_id,
        std::string response) {
        std::shared_ptr<PendingRequest> pending;

        {
            std::lock_guard lock(pending_mutex_);

            const auto iterator =
                pending_requests_.find(std::string(request_id));

            if (iterator == pending_requests_.end()) {
                return false;
            }

            pending = iterator->second;
            pending_requests_.erase(iterator);
        }

        const auto client = pending->client_session.lock();

        if (!client) {
            return true;
        }

        const auto parsed = parse_json(response);

        if (!parsed.has_value() ||
            !json_object_is_type(parsed->get(), json_type_object)) {
            (void)client->complete_client_request(
                pending->client_request_id,
                make_request_error(
                    pending->client_request_id,
                    "hub.protocol_error",
                    "Provider response could not be routed"));
            return true;
        }

        json_object_object_del(parsed->get(), "id");
        json_object_object_add(
            parsed->get(),
            "id",
            json_object_new_string(pending->client_request_id.c_str()));

        (void)client->complete_client_request(
            pending->client_request_id,
            serialize_json(parsed->get()));
        return true;
    }

    std::string listen_address_;
    asio::io_context io_context_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::atomic<bool> running_{false};
    int bound_port_{0};
    std::thread listener_thread_;
    std::thread timeout_thread_;

    ProviderRegistry provider_registry_;

    std::mutex sessions_mutex_;
    std::unordered_map<
        ProviderConnectionId,
        std::shared_ptr<Session>> sessions_;

    std::mutex session_threads_mutex_;
    std::vector<std::thread> session_threads_;

    std::mutex pending_mutex_;
    std::unordered_map<
        std::string,
        std::shared_ptr<PendingRequest>> pending_requests_;

    std::atomic<ProviderConnectionId> next_connection_id_{1};
    std::atomic<std::uint64_t> next_provider_request_id_{1};

    friend class Session;
};

ServiceHubServer::ServiceHubServer(std::string listen_address)
    : impl_(std::make_unique<Impl>(std::move(listen_address))) {}

ServiceHubServer::~ServiceHubServer() = default;

bool ServiceHubServer::start() {
    return impl_->start();
}

void ServiceHubServer::shutdown() {
    impl_->shutdown();
}

bool ServiceHubServer::running() const noexcept {
    return impl_->running();
}

int ServiceHubServer::bound_port() const noexcept {
    return impl_->bound_port();
}

std::string_view ServiceHubServer::listen_address() const noexcept {
    return impl_->listen_address();
}

}  // namespace dispatcher::service_hub
