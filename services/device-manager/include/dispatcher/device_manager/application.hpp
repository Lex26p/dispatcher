#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::device_manager {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "device-manager";
    }

    int run(std::ostream& output) const;
};

}  // namespace dispatcher::device_manager
