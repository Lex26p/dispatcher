#include <dispatcher/common/version.hpp>

#include <spdlog/spdlog.h>

int main()
{
    spdlog::info("Dispatcher smoke application");
    spdlog::info("Version: {}", dispatcher::common::version());

    return 0;
}