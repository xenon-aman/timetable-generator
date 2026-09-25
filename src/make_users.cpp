#include <iostream>
#include "auth.hpp"

int main() {
    PasswordHasher::init();

    struct Account { std::string email, password; };
    Account accounts[] = {
        {"admin@demoschool.test",   "Admin@123"},
        {"teacher@demoschool.test", "Teacher@123"},
        {"student@demoschool.test", "Student@123"}
    };

    for (auto& a : accounts) {
        std::cout << a.email << " -> " << PasswordHasher::hash(a.password) << "\n";
    }
}