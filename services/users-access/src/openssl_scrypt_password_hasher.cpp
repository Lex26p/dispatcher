#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dispatcher::users_access {
namespace {

constexpr std::uint64_t scrypt_cost_n = 1ULL << 17U;
constexpr std::uint32_t scrypt_block_size_r = 8;
constexpr std::uint32_t scrypt_parallelization_p = 1;
constexpr std::size_t salt_size = 16;
constexpr std::size_t digest_size = 32;
constexpr std::uint64_t max_memory_bytes = 256ULL * 1024ULL * 1024ULL;
constexpr std::size_t max_password_bytes = 1024;

[[nodiscard]] bool verifier_supported(const CredentialVerifier& verifier) noexcept {
    return verifier.algorithm == "scrypt" &&
           verifier.cost_n >= 2 &&
           verifier.block_size_r > 0 &&
           verifier.parallelization_p > 0 &&
           !verifier.salt.empty() &&
           !verifier.digest.empty();
}

}  // namespace

PasswordHashStatus OpenSslScryptPasswordHasher::hash(
    const std::string_view password,
    CredentialVerifier& verifier) const {
    if (password.empty() || password.size() > max_password_bytes) {
        return PasswordHashStatus::invalid_password;
    }

    std::array<unsigned char, salt_size> salt{};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        return PasswordHashStatus::crypto_error;
    }

    std::array<unsigned char, digest_size> digest{};
    if (EVP_PBE_scrypt(
            password.data(),
            password.size(),
            salt.data(),
            salt.size(),
            scrypt_cost_n,
            scrypt_block_size_r,
            scrypt_parallelization_p,
            max_memory_bytes,
            digest.data(),
            digest.size()) != 1) {
        OPENSSL_cleanse(digest.data(), digest.size());
        return PasswordHashStatus::crypto_error;
    }

    verifier.algorithm = "scrypt";
    verifier.cost_n = scrypt_cost_n;
    verifier.block_size_r = scrypt_block_size_r;
    verifier.parallelization_p = scrypt_parallelization_p;
    verifier.salt.assign(salt.begin(), salt.end());
    verifier.digest.assign(digest.begin(), digest.end());

    OPENSSL_cleanse(digest.data(), digest.size());
    return PasswordHashStatus::ok;
}

PasswordHashStatus OpenSslScryptPasswordHasher::verify(
    const std::string_view password,
    const CredentialVerifier& verifier,
    bool& matches) const {
    matches = false;

    if (password.empty() || password.size() > max_password_bytes) {
        return PasswordHashStatus::invalid_password;
    }
    if (!verifier_supported(verifier)) {
        return PasswordHashStatus::unsupported_verifier;
    }

    std::vector<unsigned char> digest(verifier.digest.size());
    if (EVP_PBE_scrypt(
            password.data(),
            password.size(),
            verifier.salt.data(),
            verifier.salt.size(),
            verifier.cost_n,
            verifier.block_size_r,
            verifier.parallelization_p,
            max_memory_bytes,
            digest.data(),
            digest.size()) != 1) {
        OPENSSL_cleanse(digest.data(), digest.size());
        return PasswordHashStatus::crypto_error;
    }

    matches = CRYPTO_memcmp(
                  digest.data(),
                  verifier.digest.data(),
                  verifier.digest.size()) == 0;
    OPENSSL_cleanse(digest.data(), digest.size());
    return PasswordHashStatus::ok;
}

}  // namespace dispatcher::users_access
