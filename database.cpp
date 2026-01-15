#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "headers/database.hpp"
#include <cstddef>
#include <nlohmann/json.hpp>
#include "headers/abstract.hpp"
#include <argon2.h>
#include <random>
#include <string>

using json = nlohmann::json;

Database::Database(const std::string& conn_str) : conn(conn_str) {}
struct User;

pqxx::connection& Database::getConnection() {
    return conn;
}

std::string generateSalt(size_t length = 16) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string salt;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) -2);
    for (size_t i = 0; i < length; ++i) {
        salt += charset[dist(rng)];
    }
    return salt;
}

std::string hashPassword(const std::string& password) {
    char hash[128];

    std::string salt = generateSalt();

    int result = argon2id_hash_encoded(
        2,
        1 << 16,
        1,
        password.c_str(),
        password.size(),
        salt.c_str(),
        salt.size(),
        32,
        hash,
        static_cast<size_t>(sizeof(hash))
    );

    if (result != ARGON2_OK) {
        throw std::runtime_error(argon2_error_message(result));
    }

    return std::string(hash);
}

bool user_exists(const std::string& username) {
    try {
        // Connect to the database
        Database db = connect_db();
        auto& conn = db.getConnection();

        // Start a transaction
        pqxx::work txn(conn);

        // Query for the username
        pqxx::result r = txn.exec("SELECT 1 FROM users WHERE username = " + txn.quote(username) + " LIMIT 1");

        // If the result has any rows, user exists
        return !r.empty();

    } catch (const std::exception &e) {
        std::cerr << "Error checking user: " << e.what() << "\n";
        return false;
    }
}

void create_account(
    const std::string& username,
    const std::string& displayName,
    const std::string& password,
    const std::string& custom_status,
    const std::string& bio
) {
    boost::uuids::uuid id = boost::uuids::random_generator()();
    std::string user_id = to_string(id);
    std::string passwrd_hash = hashPassword(password);
    std::string appearance_status = "offline";

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        if (user_exists(username)) {
            return;
        }

        pqxx::work txn(conn);
        pqxx::result r = txn.exec(
            "INSERT INTO users (username, displayname, password, user_id, appearance_status, custom_status, bio) VALUES (" 
                + txn.quote(username) + ", " 
                + txn.quote(displayName) + ", " 
                + txn.quote(passwrd_hash) + ", " 
                + txn.quote(user_id) + ", "
                + txn.quote(appearance_status) + ", "
                + txn.quote(custom_status) + ", "
                + txn.quote(bio) +
            ")"
        );

        txn.commit();
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    }
}

void update_account(const std::string& username, const std::string& displayname, const std::string& profile_picture, const std::string& custom_status, const std::string& bio, const std::string& UUID) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("UPDATE users SET username = $1, displayname = $2, profile_picture = $3, custom_status = $4, bio = $5 WHERE user_id = $6", username, displayname, profile_picture, custom_status, bio, UUID);

        txn.commit();

        std::cout << "Account updated" << "\n";

    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
    }
}

bool login_user(std::string& username, std::string& password) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec("SELECT password FROM users WHERE username = " + txn.quote(username) + " LIMIT 1");

        if (r.empty()) {
            std::cout << "User not found";
            return false;
        }

        std::string uPassword = r[0]["password"].c_str();

        if (argon2id_verify(uPassword.c_str(), password.c_str(), password.size()) == ARGON2_OK) {
            std::cout << "Login successful!\n";
            return true;
        } else {
            std::cout << "Incorrect password!\n";
            return false;
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
        return json{
            {"error", e.what()}
        };
    }
}

json get_user(const std::string& username) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec("SELECT * FROM users WHERE username = " + txn.quote(username));

        if (r.empty()) {
            std::cout << "User not found";
            return json{{"status", "failed"}};
        } else {
            std::string displayname = r[0]["displayname"].c_str();
            std::string user = r[0]["username"].c_str();
            std::string user_id = r[0]["user_id"].c_str();

            return json{
                {"username", user},
                {"displayname", displayname},
                {"user_id", user_id}
            };
        }

        txn.commit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return json{
            {"error", e.what()}
        };
    }
}

json get_user_all(const std::string& UUID) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT * FROM users WHERE user_id = $1 ", UUID);

        if (r.empty()) {
            std::cout << "User not found";
            return json{{"status", "failed"}};
        } else {
            std::string displayname = r[0]["displayname"].c_str();
            std::string username = r[0]["username"].c_str();
            std::string user_id = r[0]["user_id"].c_str();
            std::string pfp = r[0]["profile_picture"].c_str();
            std::string custom_status = r[0]["custom_status"].c_str();
            std::string bio = r[0]["bio"].c_str();

            return json{
                {"username", username},
                {"userid", user_id},
                {"displayName", displayname},
                {"picture", pfp},
                {"customStatus", custom_status},
                {"bio", bio}
            };
        }

        txn.commit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return json{
            {"what", e.what()}
        };
    }
}

json user_get_all_servers(const std::string& UUID) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(
            "SELECT s.server_id, s.server_name, s.owner "
            "FROM servers s "
            "JOIN user_servers us ON s.server_id = us.sid "
            "WHERE us.uid = $1",
            UUID
        );

        for (auto row : r) {
            std::string serverId = row["server_id"].as<std::string>();
            std::string serverName = row["server_name"].c_str();  // or row["name"].as<std::string>()
            std::string owner = row["owner"].c_str();
            response["servers"].push_back({
                {"name", serverName},
                {"serverID", serverId},
                {"owner", owner}
            });
        }
        
        txn.commit();
    } catch (std::exception &e) {
        response["error"] = 404;
        response["what"] = e.what();
        std::cout << e.what() << "\n";
    }

    return response;
}

std::string set_user_appearance_status(const std::string& UUID, const std::string& status) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("UPDATE users SET appearance_status = $1 WHERE user_id = $2 RETURNING appearance_status;", status, UUID);

        
        txn.commit();
        return r[0]["appearance_status"].as<std::string>();
    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        return "";
    }
}