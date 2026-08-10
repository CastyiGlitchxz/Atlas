#include "../include/relationships.hpp"
#include "../include/database.hpp"
#include <iostream>
#include <pqxx/internal/statement_parameters.hxx>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json send_friend_request(const std::string& Sender_UUID, const std::string& Receiver_UUID) {
    json response = json::object();

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string query =
            "INSERT INTO friendships (sender, receiver, action_user, updated_at, status) VALUES (" +
            txn.quote(Sender_UUID) + "," +
            txn.quote(Receiver_UUID) + "," +
            txn.quote(Sender_UUID) + "," +
            txn.quote("2025-11-27 14:25:00") + "," +
            txn.quote("pending") + ") RETURNING *;";
        pqxx::result r = txn.exec(query);
        txn.commit();

        response["request"]["id"] = r[0]["id"].as<int>();
        response["request"]["sender"] = r[0]["sender"].as<std::string>();
        response["request"]["receiver"] = r[0]["receiver"].as<std::string>();
        response["request"]["actionUser"] = r[0]["action_user"].as<std::string>();
        response["request"]["status"] = r[0]["status"].as<std::string>();

    } catch (std::exception& e) {
        response["error"] = e.what();
    }

    return response;
}

void change_relationship_status(int requestID, RelationshipTypes status) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string query =
            "UPDATE friendships "
            "SET status = $1 "
            "WHERE id = $2;";
        pqxx::result r = txn.exec_params(query, status, requestID);
        txn.commit();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void remove_friend(int requestID) {

}

std::optional<json> get_all_friends(const std::string& UUID) {
    json response = json::object();

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string_view query = 
            "SELECT f.sender, f.receiver, f.status, f.id, u.username, u.displayname, u.custom_status, u.appearance_status, u.bio, u.profile_picture, u.user_id "
            "FROM friendships f "
            "JOIN users u ON ( (f.sender = $1 AND u.user_id = f.receiver) OR (f.receiver = $1 AND u.user_id = f.sender) ) "
            "WHERE (f.sender = $1 OR f.receiver = $1) AND f.status = 'accepted';";
        pqxx::result r = txn.exec_params(query, UUID);

        if (r.empty())
            return nullptr;
        
        for (auto u : r) {
            std::string displayName = u["displayname"].as<std::string>();
            std::string customStatus = u["custom_status"].as<std::string>();
            std::string appearanceStatus = u["appearance_status"].as<std::string>();
            std::string profile = u["profile_picture"].as<std::string>();
            std::string userID = u["user_id"].as<std::string>();
            std::string bio = u["bio"].as<std::string>();
            int id = u["id"].as<int>();

            response["friends"].push_back({
                {"displayName", displayName},
                {"customStatus", customStatus},
                {"status", appearanceStatus},
                {"picture", profile},
                {"userID", userID},
                {"bio", bio},
                {"id", id},
            });
        }

    } catch (std::exception& e) {
        response["error"] = e.what();
    }

    return response;
}

std::optional<json> get_friend_requests(const std::string& UUID) {
    json response = json::object();

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
        
        pqxx::work txn(conn);
        std::string_view query = 
            "SELECT f.sender, f.receiver, f.status, f.id, u.username, u.displayname, u.custom_status, u.appearance_status, u.bio, u.profile_picture, u.user_id "
            "FROM friendships f "
            "JOIN users u ON ( (f.sender = $1 AND u.user_id = f.receiver) OR (f.receiver = $1 AND u.user_id = f.sender) ) "
            "WHERE (f.sender = $1 OR f.receiver = $1) AND f.status = 'pending';";
        pqxx::result r = txn.exec_params(query, UUID);

        if (r.empty())
            return nullptr;

        for (auto u : r) {
            std::string displayName = u["displayname"].as<std::string>();
            std::string customStatus = u["custom_status"].as<std::string>();
            std::string appearanceStatus = u["appearance_status"].as<std::string>();
            std::string profile = u["profile_picture"].as<std::string>();
            std::string userID = u["user_id"].as<std::string>();
            int id = u["id"].as<int>();

            response["requests"].push_back({
                {"displayName", displayName},
                {"customStatus", customStatus},
                {"appearanceStatus", appearanceStatus},
                {"picture", profile},
                {"userID", userID},
                {"id", id},
            });
        }

    } catch(const std::exception& e) {
        response["error"] = e.what();
    }

    return response;
}