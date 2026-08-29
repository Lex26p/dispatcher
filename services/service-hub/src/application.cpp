#include "dispatcher/service_hub/application.hpp"

#include <ostream>

namespace dispatcher::service_hub {

int Application::run(std::ostream& output) const {
    output << "Dispatcher Service Hub\n";
    return 0;
}

}  // namespace dispatcher::service_hub
