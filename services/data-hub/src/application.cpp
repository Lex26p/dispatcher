#include "dispatcher/data_hub/application.hpp"

#include <ostream>

namespace dispatcher::data_hub {

int Application::run(std::ostream& output) const {
    output << "Dispatcher Data Hub\n";
    return 0;
}

}  // namespace dispatcher::data_hub
