#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::service_hub {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "service-hub";
    }

    int run(std::ostream& output) const;
};

}  // namespace dispatcher::service_hub
