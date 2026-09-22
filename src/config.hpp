#pragma once
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

struct AppConfig {
    std::string db_host;
    int db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;

    static AppConfig load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + path);
        }
        nlohmann::json j;
        file >> j;

        AppConfig cfg;
        cfg.db_host     = j.at("db_host").get<std::string>();
        cfg.db_port     = j.at("db_port").get<int>();
        cfg.db_name     = j.at("db_name").get<std::string>();
        cfg.db_user     = j.at("db_user").get<std::string>();
        cfg.db_password = j.at("db_password").get<std::string>();
        return cfg;
    }
};