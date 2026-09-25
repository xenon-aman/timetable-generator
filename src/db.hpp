#pragma once
#include <mysql.h>
#include <cstring>
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

    // Safe version: use ? as a placeholder, and pass the actual value separately.
    // MySQL guarantees the value is never treated as SQL code, no matter what it contains.
    std::vector<std::map<std::string, std::string>> queryPrepared(
            const std::string& sql, const std::vector<std::string>& params) {

        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");

        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Prepare failed: " + err);
        }

        std::vector<MYSQL_BIND> binds(params.size());
        memset(binds.data(), 0, sizeof(MYSQL_BIND) * binds.size());
        std::vector<unsigned long> lengths(params.size());

        for (size_t i = 0; i < params.size(); i++) {
            lengths[i] = params[i].size();
            binds[i].buffer_type = MYSQL_TYPE_STRING;
            binds[i].buffer = (void*)params[i].c_str();
            binds[i].buffer_length = lengths[i];
            binds[i].length = &lengths[i];
        }

        if (!params.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Bind failed: " + err);
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Execute failed: " + err);
        }

        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        std::vector<std::map<std::string, std::string>> rows;

        if (meta) {
            unsigned int numFields = mysql_num_fields(meta);
            MYSQL_FIELD* fields = mysql_fetch_fields(meta);

            std::vector<std::vector<char>> buffers(numFields, std::vector<char>(1024));
            std::vector<unsigned long> outLengths(numFields);
            std::vector<my_bool> isNull(numFields);
            std::vector<MYSQL_BIND> outBinds(numFields);
            memset(outBinds.data(), 0, sizeof(MYSQL_BIND) * numFields);

            for (unsigned int i = 0; i < numFields; i++) {
                outBinds[i].buffer_type = MYSQL_TYPE_STRING;
                outBinds[i].buffer = buffers[i].data();
                outBinds[i].buffer_length = buffers[i].size();
                outBinds[i].length = &outLengths[i];
                outBinds[i].is_null = &isNull[i];
            }
            mysql_stmt_bind_result(stmt, outBinds.data());

            while (mysql_stmt_fetch(stmt) == 0) {
                std::map<std::string, std::string> rowMap;
                for (unsigned int i = 0; i < numFields; i++) {
                    rowMap[fields[i].name] = isNull[i] ? "" : std::string(buffers[i].data(), outLengths[i]);
                }
                rows.push_back(rowMap);
            }
            mysql_free_result(meta);
        }

        mysql_stmt_close(stmt);
        return rows;
    }

private:
    MYSQL* conn;
};