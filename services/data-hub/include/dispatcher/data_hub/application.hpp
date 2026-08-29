#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::data_hub {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "data-hub";
    }

    int run(
        std::ostream& output,
        std::string_view listen_address = "0.0.0.0:50051") const;
};

}  // namespace dispatcher::data_hub
