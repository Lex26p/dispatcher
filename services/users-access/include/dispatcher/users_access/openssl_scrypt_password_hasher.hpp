#pragma once

#include "dispatcher/users_access/credential.hpp"

namespace dispatcher::users_access {

class OpenSslScryptPasswordHasher final : public PasswordHasher {
public:
    [[nodiscard]] PasswordHashStatus hash(
        std::string_view password,
        CredentialVerifier& verifier) const override;

    [[nodiscard]] PasswordHashStatus verify(
        std::string_view password,
        const CredentialVerifier& verifier,
        bool& matches) const override;
};

}  // namespace dispatcher::users_access
