#include "dispatcher/data_hub/application.hpp"

#include "dispatcher/data_hub/server.hpp"

#include <ostream>
#include <string>

namespace dispatcher::data_hub {

int Application::run(
    std::ostream& output,
    const std::string_view listen_address) const {
    DataHubServer server(std::string(listen_address));

    if (!server.start()) {
        output << "Failed to start Dispatcher Data Hub on "
               << listen_address << '\n';
        return 1;
    }

    output << "Dispatcher Data Hub listening on "
           << listen_address << '\n';
    output.flush();

    server.wait();
    return 0;
}

}  // namespace dispatcher::data_hub
