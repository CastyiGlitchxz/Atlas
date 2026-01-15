#include "headers/database.hpp"
#include "headers/messaging.hpp"
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>
#include "headers/abstract.hpp"
#include <yaml-cpp/yaml.h>

using json = nlohmann::json;

struct Message {
    int id;
    std::string server_id;
    std::string user_id;
    std::string content;
    std::string timestamp;
    std::optional<int> message_ref;
    std::optional<std::string> link;
};

std::string getCurrentTimestamp() {
    // Get current system time
    auto now = std::chrono::system_clock::now();
    
    // Convert to time_t (calendar time)
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure for local time
    std::tm* tm_ptr = std::localtime(&t); // or std::gmtime(&t) for UTC

    // Format as SQL DATETIME: YYYY-MM-DD HH:MM:SS
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

std::tm parseTimestamp(const std::string& ts) {
    std::tm tm = {};
    std::istringstream ss(ts);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S"); // 24-hour input
    return tm;
}

json get_user_by_UUID(const std::string& UUID) {
    json result;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT displayname FROM users WHERE user_id = $1", UUID);


        return r[0]["displayname"].as<std::string>();

        txn.commit();
    } catch (std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        std::cerr << e.what() << "\n";
    }

    return result;
}

json get_user_all(const std::string& UUID);

json get_messages(const std::string serverID, std::optional<int> index) {
    static int messageLimit = YAML::LoadFile("../config/app-config.yml")["chat"]["message_limit"].as<int>();

    json result;
    result["messages"] = json::array();
    result["status"] = true;

    std::vector<json> messageList;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
        pqxx::nontransaction txn(conn);

        std::string query = 
            "SELECT m.id, m.server_id, m.user_id, m.content, m.timestamp, u.displayname, u.profile_picture, m.message_ref, m.link "
            "FROM messages m "
            "JOIN users u ON m.user_id = u.user_id " // Fetch user data in the same breath
            "WHERE m.server_id = " + txn.quote(serverID);

        if (index.has_value()) {
            query += " AND m.id < " + txn.quote(index.value());
        }
        
        query += " ORDER BY m.id DESC LIMIT " + txn.quote(messageLimit) + ";";

        pqxx::result r = txn.exec(query);
        
        for (auto row : r) {
            std::tm tm = parseTimestamp(r[0]["timestamp"].as<std::string>());
            std::ostringstream oss;
            oss << std::put_time(&tm, "%I:%M %p"); // 12-hour with AM/PM

            std::optional<int> message_ref = row["message_ref"].as<std::optional<int>>();
            std::optional<std::string> link = row["link"].as<std::optional<std::string>>();

            json message;
            message["id"] = row["id"].as<int>();
            message["server_id"] = row["server_id"].as<std::string>();
            message["displayName"] = row["displayName"].as<std::string>();
            message["picture"] = row["profile_picture"].as<std::string>();
            message["content"] = row["content"].as<std::string>();;
            message["timestamp"] = oss.str();
            message["messageRef"] = row["message_ref"].is_null() ? json(nullptr) : json(row["message_ref"].as<int>());
            message["link"] = row["link"].is_null() ? json(nullptr) : json(row["link"].as<std::string>());

            messageList.push_back(message);
        }
        
        std::reverse(messageList.begin(), messageList.end());
        
        for (auto& m : messageList) {
            result["messages"].push_back(m);
        }

    } catch (const std::exception& e) {
        result["status"] = false;
        result["what"] = e.what();
    }

    return result;
}

json create_message(const std::string& user_id, const MessageFormat& message) {
    json result;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn); // transaction

        auto time = getCurrentTimestamp();

        std::string sql = "INSERT INTO messages (user_id, content, server_id, timestamp, message_ref, link) VALUES (" +
            txn.quote(user_id) + ", " +
            txn.quote(message.content) + ", " +
            txn.quote(message.serverID) + ", " +
            txn.quote(time) + ", " +
            txn.quote(message.messageRef) + ", " +
            txn.quote(message.link) + ") RETURNING id, timestamp;";

        std::cout << "[DEBUG] SQL: " << sql << "\n";

        pqxx::result r = txn.exec(sql);
        txn.commit();

        if (!r.empty()) {
            std::string message_id = r[0]["id"].as<std::string>();
            std::tm tm = parseTimestamp(r[0]["timestamp"].as<std::string>());

            std::ostringstream oss;
            oss << std::put_time(&tm, "%I:%M %p"); // 12-hour with AM/PM
            std::string time12h = oss.str();

            result["id"] = message_id;
            result["timestamp"] = time12h;
            result["success"] = true;
            result["message"] = "Message added successfully";
        } else {
            result["success"] = false;
            result["error"] = "No ID returned";
        }

    } catch (const pqxx::sql_error &e) {
        std::cerr << "[SQL ERROR] " << e.what() << "\nQuery: " << e.query() << "\n";
        result["success"] = false;
        result["error"] = e.what();
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        result["success"] = false;
        result["error"] = e.what();
    }

    return result;
}

json delete_message(int message_id) {
    json result;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        txn.exec_params("DELETE FROM messages WHERE id = $1", message_id);
        txn.commit();

        result["success"] = true;
        result["message"] = "Message deleted successfully";
    } catch(const std::exception &e) {
        result["success"] = false;
        result["error"] = e.what();
    }

    return result;
}

json edit_message(int message_id, std::string& content) {
    json result;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        txn.exec_params("UPDATE messages SET content = $1 WHERE id = $2", content, message_id);
        txn.commit();

        result["success"] = true;
        result["message"] = "Message edited successfully";
    } catch(const std::exception &e) {
        result["success"] = false;
        result["error"] = e.what();
    }

    return result;
}