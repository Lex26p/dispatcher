#include "dispatcher/users_access/application.hpp"

#include <csignal>
#include <optional>
#include <ostream>
#include <pthread.h>
#include <signal.h>
#include <string_view>

namespace dispatcher::users_access {
namespace {

class ShutdownSignalWaiter final {
public:
    ShutdownSignalWaiter() {
        sigemptyset(&wait_set_);
        sigaddset(&wait_set_, SIGINT);
        sigaddset(&wait_set_, SIGTERM);
        ready_ = pthread_sigmask(SIG_BLOCK, &wait_set_, &previous_mask_) == 0;
    }

    ~ShutdownSignalWaiter() {
        if (ready_) {
            pthread_sigmask(SIG_SETMASK, &previous_mask_, nullptr);
        }
    }

    ShutdownSignalWaiter(const ShutdownSignalWaiter&) = delete;
    ShutdownSignalWaiter& operator=(const ShutdownSignalWaiter&) = delete;

    [[nodiscard]] bool ready() const noexcept {
        return ready_;
    }

    [[nodiscard]] std::optional<int> wait() const {
        if (!ready_) {
            return std::nullopt;
        }

        int signal_number = 0;
        if (sigwait(&wait_set_, &signal_number) != 0) {
            return std::nullopt;
        }
        return signal_number;
    }

    [[nodiscard]] static std::string_view name(const int signal_number) noexcept {
        switch (signal_number) {
        case SIGINT:
            return "SIGINT";
        case SIGTERM:
            return "SIGTERM";
        default:
            return "unknown signal";
        }
    }

private:
    sigset_t wait_set_{};
    sigset_t previous_mask_{};
    bool ready_{false};
};

}  // namespace

int Application::run(
    std::ostream& output,
    const std::string_view database_path) const {
    const ShutdownSignalWaiter shutdown_signal;
    if (!shutdown_signal.ready()) {
        output << "Failed to configure Dispatcher Users & Access shutdown signals\n";
        return 1;
    }

    output << "Dispatcher Users & Access started "
           << "(SQLite " << database_path
           << "; authentication/session contract not configured)\n";
    output.flush();

    const auto signal_number = shutdown_signal.wait();
    if (!signal_number.has_value()) {
        output << "Failed to wait for Dispatcher Users & Access shutdown signal\n";
        output.flush();
        return 1;
    }

    output << "Dispatcher Users & Access shutdown requested by "
           << ShutdownSignalWaiter::name(*signal_number) << '\n';
    output << "Dispatcher Users & Access stopped\n";
    output.flush();
    return 0;
}

}  // namespace dispatcher::users_access
