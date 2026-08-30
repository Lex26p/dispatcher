#pragma once

namespace dispatcher::users_access {

class UsersAccessServiceProvider {
public:
    virtual ~UsersAccessServiceProvider() = default;

    [[nodiscard]] virtual bool start() = 0;
    virtual void stop() = 0;
};

}  // namespace dispatcher::users_access
