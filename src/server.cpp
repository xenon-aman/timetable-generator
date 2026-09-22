#include <iostream>
#include "httplib.h"
#include "config.hpp"
#include "db.hpp"
#include <nlohmann/json.hpp>

int main() {
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

        std::cout << "Step 4: about to call listen() on 0.0.0.0:8080" << std::endl;
        bool ok = svr.listen("0.0.0.0", 8080);
        std::cout << "Step 5: listen() returned: " << ok << std::endl;

    } catch (const std::exception& e) {
        std::cout << "CRASHED WITH ERROR: " << e.what() << std::endl;
    }
    std::cout << "Step 6: main() ending" << std::endl;
}