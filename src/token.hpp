#pragma once
#define JWT_DISABLE_PICOJSON
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <string>
#include <stdexcept>

class TokenService {
public:
    TokenService(const std::string& secret) : secret(secret) {}

    // Creates a signed token containing the user's id, role, and institution.
    // Expires in 8 hours.
    std::string createToken(const std::string& userId, const std::string& role,
                             const std::string& institutionId) {
        auto now = std::chrono::system_clock::now();
        return jwt::create<jwt::traits::nlohmann_json>()
            .set_type("JWT")
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::hours(8))
            .set_payload_claim("user_id", jwt::traits::nlohmann_json::value_type(userId))
            .set_payload_claim("role", jwt::traits::nlohmann_json::value_type(role))
            .set_payload_claim("institution_id", jwt::traits::nlohmann_json::value_type(institutionId))
            .sign(jwt::algorithm::hs256{secret});
    }

    // Checks a token's signature and expiry. Throws if invalid.
    // Returns the decoded token so the caller can read its claims.
    jwt::decoded_jwt<jwt::traits::nlohmann_json> verifyToken(const std::string& token) {
        auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);

        auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
            .allow_algorithm(jwt::algorithm::hs256{secret});

        verifier.verify(decoded);  // throws jwt::error::token_verification_exception if invalid/expired
        return decoded;
    }

private:
    std::string secret;
};