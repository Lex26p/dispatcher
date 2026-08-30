#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dispatcher::users_access {

struct CredentialVerifier final {
    std::string user_id;
    std::string algorithm;
    std::uint64_t cost_n{0};
    std::uint32_t block_size_r{0};
    std::uint32_t parallelization_p{0};
    std::vector<unsigned char> salt;
    std::vector<unsigned char> digest;
};

enum class CredentialRepositoryStatus {
    ok,
    not_found,
    conflict,
    error,
};

class CredentialRepository {
public:
    virtual ~CredentialRepository() = default;

    virtual CredentialRepositoryStatus set_credential_verifier(
        const CredentialVerifier& verifier) = 0;

    virtual CredentialRepositoryStatus find_credential_verifier(
        std::string_view user_id,
        CredentialVerifier& verifier) const = 0;
};

enum class PasswordHashStatus {
    ok,
    invalid_password,
    unsupported_verifier,
    crypto_error,
};

class PasswordHasher {
public:
    virtual ~PasswordHasher() = default;

    virtual PasswordHashStatus hash(
        std::string_view password,
        CredentialVerifier& verifier) const = 0;

    virtual PasswordHashStatus verify(
        std::string_view password,
        const CredentialVerifier& verifier,
        bool& matches) const = 0;
};

}  // namespace dispatcher::users_access
