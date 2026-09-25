#include <iostream>
#include "httplib.h"
#include "config.hpp"
#include "db.hpp"
#include "auth.hpp"
#include <nlohmann/json.hpp>

int main() {
    PasswordHasher::init();
    std::cout << "Step 1: starting" << std::endl;
    try {
        AppConfig cfg = AppConfig::load("config.json");
        std::cout << "Step 2: config loaded" << std::endl;

        Database db(cfg.db_host, cfg.db_port, cfg.db_user, cfg.db_password, cfg.db_name);
        std::cout << "Step 3: connected to database: " << cfg.db_name << std::endl;

        httplib::Server svr;

        svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("<h1>Timetable Generator is running!</h1>", "text/html");
        });

        svr.Get("/api/v1/classes", [&db](const httplib::Request&, httplib::Response& res) {
            try {
                auto rows = db.query("SELECT id, name, strength FROM classes ORDER BY name");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : rows) {
                    arr.push_back({
                        {"id", row["id"]},
                        {"name", row["name"]},
                        {"strength", row["strength"]}
                    });
                }
                res.set_content(arr.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        svr.Post("/api/v1/login", [&db](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string email = body.at("email").get<std::string>();
                std::string password = body.at("password").get<std::string>();

               auto rows = db.queryPrepared(
                    "SELECT id, password_hash, role, institution_id FROM users WHERE email = ?",
                    {email}
                );

                if (rows.empty()) {
                    res.status = 401;
                    res.set_content("{\"error\":\"Invalid email or password\"}", "application/json");
                    return;
                }

                auto& user = rows[0];
                if (!PasswordHasher::verify(password, user["password_hash"])) {
                    res.status = 401;
                    res.set_content("{\"error\":\"Invalid email or password\"}", "application/json");
                    return;
                }

                nlohmann::json result = {
                    {"id", user["id"]},
                    {"role", user["role"]},
                    {"institution_id", user["institution_id"]}
                };
                res.set_content(result.dump(), "application/json");

            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        std::cout << "Step 4: about to call listen() on 0.0.0.0:8080" << std::endl;
        bool ok = svr.listen("0.0.0.0", 8080);
        std::cout << "Step 5: listen() returned: " << ok << std::endl;

    } catch (const std::exception& e) {
        std::cout << "CRASHED WITH ERROR: " << e.what() << std::endl;
    }
    std::cout << "Step 6: main() ending" << std::endl;
}