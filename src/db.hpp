#pragma once
#include <mysql.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

class Database {
public:
    Database(const std::string& host, int port, const std::string& user,
             const std::string& password, const std::string& dbname) {
        conn = mysql_init(nullptr);
        if (!conn) throw std::runtime_error("mysql_init failed");

        if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
                                 password.c_str(), dbname.c_str(),
                                 port, nullptr, 0)) {
            std::string err = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL connect failed: " + err);
        }
    }

    ~Database() {
        if (conn) mysql_close(conn);
    }

    // Runs a SELECT and returns rows as a list of column-name -> value maps
    std::vector<std::map<std::string, std::string>> query(const std::string& sql) {
        if (mysql_query(conn, sql.c_str()) != 0) {
            throw std::runtime_error("Query failed: " + std::string(mysql_error(conn)));
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (!result) {
            throw std::runtime_error("mysql_store_result failed: " + std::string(mysql_error(conn)));
        }

        unsigned int numFields = mysql_num_fields(result);
        MYSQL_FIELD* fields = mysql_fetch_fields(result);

        std::vector<std::map<std::string, std::string>> rows;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::map<std::string, std::string> rowMap;
            for (unsigned int i = 0; i < numFields; i++) {
                rowMap[fields[i].name] = row[i] ? row[i] : "";
            }
            rows.push_back(rowMap);
        }

        mysql_free_result(result);
        return rows;
    }

private:
    MYSQL* conn;
};