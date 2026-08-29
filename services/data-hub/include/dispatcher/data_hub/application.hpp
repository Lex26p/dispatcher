#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::data_hub {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "data-hub";
    }

    int run(std::ostream& output) const;
};

}  // namespace dispatcher::data_hub
