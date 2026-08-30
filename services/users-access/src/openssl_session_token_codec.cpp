#include "dispatcher/users_access/openssl_session_token_codec.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::users_access {
namespace {

[[nodiscard]] char hex_digit(const unsigned int value) noexcept {
    static constexpr char digits[] = "0123456789abcdef";
    return digits[value & 0x0FU];
}

[[nodiscard]] int hex_value(const char character) noexcept {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return 10 + (character - 'a');
    }
    return -1;
}

[[nodiscard]] bool sha256(
    const unsigned char* data,
    const std::size_t size,
    std::vector<unsigned char>& digest) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> buffer{};
    unsigned int digest_size = 0;
    if (EVP_Digest(
            data,
            size,
            buffer.data(),
            &digest_size,
            EVP_sha256(),
            nullptr) != 1 ||
        digest_size != session_token_digest_bytes) {
        digest.clear();
        return false;
    }

    digest.assign(buffer.begin(), buffer.begin() + digest_size);
    return true;
}

}  // namespace

SessionTokenStatus OpenSslSessionTokenCodec::generate(
    std::string& token,
    std::vector<unsigned char>& token_digest) const {
    std::array<unsigned char, session_token_bytes> raw{};
    if (RAND_bytes(raw.data(), static_cast<int>(raw.size())) != 1) {
        token.clear();
        token_digest.clear();
        return SessionTokenStatus::crypto_error;
    }

    token.clear();
    token.reserve(raw.size() * 2U);
    for (const auto byte : raw) {
        token.push_back(hex_digit(byte >> 4U));
        token.push_back(hex_digit(byte));
    }

    if (!sha256(raw.data(), raw.size(), token_digest)) {
        token.clear();
        return SessionTokenStatus::crypto_error;
    }
    return SessionTokenStatus::ok;
}

SessionTokenStatus OpenSslSessionTokenCodec::digest(
    const std::string_view token,
    std::vector<unsigned char>& token_digest) const {
    if (token.size() != session_token_bytes * 2U) {
        token_digest.clear();
        return SessionTokenStatus::invalid_token;
    }

    std::array<unsigned char, session_token_bytes> raw{};
    for (std::size_t index = 0; index < raw.size(); ++index) {
        const int high = hex_value(token[index * 2U]);
        const int low = hex_value(token[index * 2U + 1U]);
        if (high < 0 || low < 0) {
            token_digest.clear();
            return SessionTokenStatus::invalid_token;
        }
        raw[index] = static_cast<unsigned char>((high << 4) | low);
    }

    if (!sha256(raw.data(), raw.size(), token_digest)) {
        return SessionTokenStatus::crypto_error;
    }
    return SessionTokenStatus::ok;
}

}  // namespace dispatcher::users_access
