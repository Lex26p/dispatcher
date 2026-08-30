#pragma once

#include "dispatcher/users_access/session.hpp"

namespace dispatcher::users_access {

class OpenSslSessionTokenCodec final : public SessionTokenCodec {
public:
    [[nodiscard]] SessionTokenStatus generate(
        std::string& token,
        std::vector<unsigned char>& token_digest) const override;

    [[nodiscard]] SessionTokenStatus digest(
        std::string_view token,
        std::vector<unsigned char>& token_digest) const override;
};

}  // namespace dispatcher::users_access
