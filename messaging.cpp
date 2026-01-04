#include "headers/database.hpp"
#include "headers/messaging.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>
#include "headers/abstract.hpp"

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
    json result;
    std::vector<Message> messages;
    int messageLimit = 25;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::nontransaction txn(conn);

        pqxx::result r_init;

        if (!index.has_value()) {
            r_init = txn.exec(
                "SELECT id, server_id, user_id, content, timestamp, message_ref, link "
                "FROM messages WHERE server_id = " + txn.quote(serverID) +
                " ORDER BY id DESC LIMIT " + txn.quote(messageLimit) + ";"
            );
        } else {
            r_init = txn.exec(
                "SELECT id, server_id, user_id, content, timestamp, message_ref, link "
                "FROM messages WHERE server_id = " + txn.quote(serverID) +
                "AND id < " + txn.quote(index) +
                " ORDER BY id DESC LIMIT " + txn.quote(messageLimit) + ";"
            );
        }
        

        messages.reserve(r_init.size());

        std::tm tm = parseTimestamp(r_init[0]["timestamp"].as<std::string>());

        std::ostringstream oss;
        oss << std::put_time(&tm, "%I:%M %p"); // 12-hour with AM/PM
        std::string time12h = oss.str();

        for (auto row : r_init) {
            std::optional<int> message_ref = row["message_ref"].as<std::optional<int>>();
            std::optional<std::string> link = row["link"].as<std::optional<std::string>>();

            Message msg {
                row["id"].as<int>(),
                row["server_id"].as<std::string>(),
                row["user_id"].as<std::string>(),
                row["content"].as<std::string>(),
                time12h,
                message_ref,
                link
            };
            messages.push_back(msg);
        }

        std::reverse(messages.begin(), messages.end());

        result["success"] = true;
        result["messages"] = json::array();

        for (auto& msg : messages) {
            json user = get_user_all(msg.user_id);
            std::string displayName = user.value("displayName", "");
            std::string pfp = user.value("picture", "");

            json message;
            message["id"] = msg.id;
            message["server_id"] = msg.server_id;
            message["displayName"] = displayName;
            message["picture"] = pfp;
            message["content"] = msg.content;
            message["timestamp"] = msg.timestamp;
            message["messageRef"] = msg.message_ref ? json(*msg.message_ref) : json(nullptr);
            message["link"] = msg.link ? json(*msg.link) : json(nullptr);

            result["messages"].push_back(message);
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
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