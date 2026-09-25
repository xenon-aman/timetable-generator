#pragma once
#include <sodium.h>
#include <string>
#include <stdexcept>

class PasswordHasher {
public:
    // Call this once, at the very start of main(), before anything else uses sodium
    static void init() {
        if (sodium_init() < 0) {
            throw std::runtime_error("libsodium failed to initialize");
        }
    }

    // Turns a plain password into a safe, storable hash (includes its own salt)
    static std::string hash(const std::string& password) {
        char hashed[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(
                hashed,
                password.c_str(), password.size(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
            throw std::runtime_error("Password hashing failed (out of memory?)");
        }
        return std::string(hashed);
    }

    // Checks a plain password against a stored hash. Returns true if it matches.
    static bool verify(const std::string& password, const std::string& storedHash) {
        return crypto_pwhash_str_verify(
                   storedHash.c_str(),
                   password.c_str(), password.size()) == 0;
    }
};