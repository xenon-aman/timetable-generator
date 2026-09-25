#include <iostream>
#include "auth.hpp"

int main() {
    PasswordHasher::init();

    std::string pw = "MySecret123";
    std::string h = PasswordHasher::hash(pw);

    std::cout << "Hash: " << h << "\n";
    std::cout << "Correct password check: " << PasswordHasher::verify(pw, h) << "\n";
    std::cout << "Wrong password check: " << PasswordHasher::verify("WrongPass", h) << "\n";
}