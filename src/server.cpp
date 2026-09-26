#include <iostream>
#include <optional>
#include <algorithm>
#include "httplib.h"
#include "config.hpp"
#include "db.hpp"
#include "auth.hpp"
#include "token.hpp"
#include <nlohmann/json.hpp>

int main() {
    PasswordHasher::init();
    std::cout << "Step 1: starting" << std::endl;
    try {
        AppConfig cfg = AppConfig::load("config.json");
        std::cout << "Step 2: config loaded" << std::endl;

        Database db(cfg.db_host, cfg.db_port, cfg.db_user, cfg.db_password, cfg.db_name);
        std::cout << "Step 3: connected to database: " << cfg.db_name << std::endl;

        TokenService tokens(cfg.jwt_secret);

        httplib::Server svr;
        svr.set_mount_point("/", "./public");

        // Reads the "Authorization: Bearer <token>" header, verifies it, and
        // (if allowedRoles is non-empty) checks the token's role is in that list.
        // Returns the decoded token if everything checks out, writes an
        // appropriate error response and returns std::nullopt otherwise.
        auto requireAuth = [&tokens](const httplib::Request& req, httplib::Response& res,
                                      std::vector<std::string> allowedRoles = {})
                -> std::optional<jwt::decoded_jwt<jwt::traits::nlohmann_json>> {
            auto authHeader = req.get_header_value("Authorization");
            const std::string prefix = "Bearer ";
            if (authHeader.size() <= prefix.size() || authHeader.compare(0, prefix.size(), prefix) != 0) {
                res.status = 401;
                res.set_content("{\"error\":\"Missing or malformed Authorization header\"}", "application/json");
                return std::nullopt;
            }
            std::string token = authHeader.substr(prefix.size());

            std::optional<jwt::decoded_jwt<jwt::traits::nlohmann_json>> decoded;
            try {
                decoded = tokens.verifyToken(token);
            } catch (const std::exception&) {
                res.status = 401;
                res.set_content("{\"error\":\"Invalid or expired token\"}", "application/json");
                return std::nullopt;
            }

            if (!allowedRoles.empty()) {
                std::string role = decoded->get_payload_claim("role").as_string();
                bool ok = std::find(allowedRoles.begin(), allowedRoles.end(), role) != allowedRoles.end();
                if (!ok) {
                    res.status = 403;
                    res.set_content("{\"error\":\"You do not have permission to access this\"}", "application/json");
                    return std::nullopt;
                }
            }
            return decoded;
        };

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

        svr.Post("/api/v1/login", [&db, &tokens](const httplib::Request& req, httplib::Response& res) {
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

                std::string token = tokens.createToken(user["id"], user["role"], user["institution_id"]);

                nlohmann::json result = {
                    {"token", token},
                    {"role", user["role"]},
                    {"institution_id", user["institution_id"]}
                };
                res.set_content(result.dump(), "application/json");

            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        svr.Get("/api/v1/me", [&requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res);
            if (!decoded) return;

            nlohmann::json result = {
                {"user_id", decoded->get_payload_claim("user_id").as_string()},
                {"role", decoded->get_payload_claim("role").as_string()},
                {"institution_id", decoded->get_payload_claim("institution_id").as_string()}
            };
            res.set_content(result.dump(), "application/json");
        });
                // List all subjects for the logged-in admin's institution
        svr.Get("/api/v1/subjects", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            std::string instId = decoded->get_payload_claim("institution_id").as_string();
            auto rows = db.queryPrepared(
                "SELECT id, name, is_lab FROM subjects WHERE institution_id = ? ORDER BY name",
                {instId}
            );

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : rows) {
                arr.push_back({
                    {"id", row["id"]},
                    {"name", row["name"]},
                    {"is_lab", row["is_lab"] == "1"}
                });
            }
            res.set_content(arr.dump(), "application/json");
        });

        // Create a new subject
                svr.Post("/api/v1/classes", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                int strength = body.value("strength", 0);

                db.queryPrepared(
                    "INSERT INTO classes (institution_id, name, strength) VALUES (?, ?, ?)",
                    {instId, name, std::to_string(strength)}
                );
                res.status = 201;
                res.set_content("{\"message\":\"Class created\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        svr.Put(R"(/api/v1/classes/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string classId = req.matches[1];
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                int strength = body.value("strength", 0);

                db.queryPrepared(
                    "UPDATE classes SET name = ?, strength = ? WHERE id = ? AND institution_id = ?",
                    {name, std::to_string(strength), classId, instId}
                );
                res.set_content("{\"message\":\"Class updated\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

                svr.Delete(R"(/api/v1/subjects/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string subjectId = req.matches[1];

                db.queryPrepared(
                    "DELETE FROM subjects WHERE id = ? AND institution_id = ?",
                    {subjectId, instId}
                );
                res.set_content("{\"message\":\"Subject deleted\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 409;
                res.set_content("{\"error\":\"Cannot delete this subject, it is still used in one or more assignments\"}", "application/json");
            }
        });
        svr.Post("/api/v1/subjects", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                bool isLab = body.value("is_lab", false);

                db.queryPrepared(
                    "INSERT INTO subjects (institution_id, name, is_lab) VALUES (?, ?, ?)",
                    {instId, name, isLab ? "1" : "0"}
                );
                res.status = 201;
                res.set_content("{\"message\":\"Subject created\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        // Update an existing subject
        svr.Put(R"(/api/v1/subjects/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string subjectId = req.matches[1];
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                bool isLab = body.value("is_lab", false);

                db.queryPrepared(
                    "UPDATE subjects SET name = ?, is_lab = ? WHERE id = ? AND institution_id = ?",
                    {name, isLab ? "1" : "0", subjectId, instId}
                );
                res.set_content("{\"message\":\"Subject updated\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        // Delete a subject
        svr.Delete(R"(/api/v1/subjects/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            std::string instId = decoded->get_payload_claim("institution_id").as_string();
            std::string subjectId = req.matches[1];

            db.queryPrepared(
                "DELETE FROM subjects WHERE id = ? AND institution_id = ?",
                {subjectId, instId}
            );
            res.set_content("{\"message\":\"Subject deleted\"}", "application/json");
        });
                svr.Get("/api/v1/teachers", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            std::string instId = decoded->get_payload_claim("institution_id").as_string();
            auto rows = db.queryPrepared(
                "SELECT id, name, max_periods_day FROM teachers WHERE institution_id = ? ORDER BY name",
                {instId}
            );

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : rows) {
                arr.push_back({
                    {"id", row["id"]},
                    {"name", row["name"]},
                    {"max_periods_day", row["max_periods_day"]}
                });
            }
            res.set_content(arr.dump(), "application/json");
        });

        svr.Post("/api/v1/teachers", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                int maxPeriods = body.value("max_periods_day", 6);

                db.queryPrepared(
                    "INSERT INTO teachers (institution_id, name, max_periods_day) VALUES (?, ?, ?)",
                    {instId, name, std::to_string(maxPeriods)}
                );
                res.status = 201;
                res.set_content("{\"message\":\"Teacher created\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        svr.Put(R"(/api/v1/teachers/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string teacherId = req.matches[1];
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                int maxPeriods = body.value("max_periods_day", 6);

                db.queryPrepared(
                    "UPDATE teachers SET name = ?, max_periods_day = ? WHERE id = ? AND institution_id = ?",
                    {name, std::to_string(maxPeriods), teacherId, instId}
                );
                res.set_content("{\"message\":\"Teacher updated\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

                svr.Delete(R"(/api/v1/teachers/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string teacherId = req.matches[1];

                db.queryPrepared(
                    "DELETE FROM teachers WHERE id = ? AND institution_id = ?",
                    {teacherId, instId}
                );
                res.set_content("{\"message\":\"Teacher deleted\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 409;
                res.set_content("{\"error\":\"Cannot delete this teacher, they are still assigned to one or more classes\"}", "application/json");
            }
        });
                svr.Get("/api/v1/rooms", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            std::string instId = decoded->get_payload_claim("institution_id").as_string();
            auto rows = db.queryPrepared(
                "SELECT id, name, room_type, capacity FROM rooms WHERE institution_id = ? ORDER BY name",
                {instId}
            );

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : rows) {
                arr.push_back({
                    {"id", row["id"]},
                    {"name", row["name"]},
                    {"room_type", row["room_type"]},
                    {"capacity", row["capacity"]}
                });
            }
            res.set_content(arr.dump(), "application/json");
        });

        svr.Post("/api/v1/rooms", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                std::string roomType = body.value("room_type", "classroom");
                int capacity = body.at("capacity").get<int>();

                db.queryPrepared(
                    "INSERT INTO rooms (institution_id, name, room_type, capacity) VALUES (?, ?, ?, ?)",
                    {instId, name, roomType, std::to_string(capacity)}
                );
                res.status = 201;
                res.set_content("{\"message\":\"Room created\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

        svr.Put(R"(/api/v1/rooms/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string roomId = req.matches[1];
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.at("name").get<std::string>();
                std::string roomType = body.value("room_type", "classroom");
                int capacity = body.at("capacity").get<int>();

                db.queryPrepared(
                    "UPDATE rooms SET name = ?, room_type = ?, capacity = ? WHERE id = ? AND institution_id = ?",
                    {name, roomType, std::to_string(capacity), roomId, instId}
                );
                res.set_content("{\"message\":\"Room updated\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
            }
        });

                svr.Delete(R"(/api/v1/rooms/(\d+))", [&db, &requireAuth](const httplib::Request& req, httplib::Response& res) {
            auto decoded = requireAuth(req, res, {"admin"});
            if (!decoded) return;

            try {
                std::string instId = decoded->get_payload_claim("institution_id").as_string();
                std::string roomId = req.matches[1];

                db.queryPrepared(
                    "DELETE FROM rooms WHERE id = ? AND institution_id = ?",
                    {roomId, instId}
                );
                res.set_content("{\"message\":\"Room deleted\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 409;
                res.set_content("{\"error\":\"Cannot delete this room, it is still used in a timetable\"}", "application/json");
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