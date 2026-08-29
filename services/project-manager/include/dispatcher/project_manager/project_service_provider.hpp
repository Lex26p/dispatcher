#pragma once

namespace dispatcher::project_manager {

class ProjectServiceProvider {
public:
    virtual ~ProjectServiceProvider() = default;

    [[nodiscard]] virtual bool start() = 0;
    virtual void stop() = 0;
};

}  // namespace dispatcher::project_manager
