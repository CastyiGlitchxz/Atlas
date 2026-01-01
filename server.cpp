#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "headers/database.hpp"
#include <exception>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

json server_get_all_users(const std::string server_id) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(
            "SELECT u.displayname, u.profile_picture, u.appearance_status, u.custom_status, u.user_id, u.bio "
            "FROM users u "
            "JOIN user_servers us ON u.user_id = us.uid "
            "WHERE us.sid = $1",
            server_id
        );

        for (auto row : r) {
            std::string displayname = row["displayname"].as<std::string>();
            std::string picture = row["profile_picture"].as<std::string>();
            std::string status = row["appearance_status"].as<std::string>();
            std::string uscms = row["custom_status"].as<std::string>();
            std::string user_id = row["user_id"].as<std::string>();
            std::string bio = row["bio"].as<std::string>();

            response["user_list"].push_back({
                {"displayName", displayname},
                {"status", status},
                {"picture", picture},
                {"customStatus", uscms},
                {"userid", user_id},
                {"bio", bio}
            });
            response["status"] = 200;
        };

    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        response["what"] = e.what();
        response["status"] = 404;
    }

    return response;
}

json get_server(const std::string server_id) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT * FROM servers WHERE server_id = $1", server_id);

        if (r.empty()) {
            std::cout << "Server not found" << "\n";
            return json{{"status", "failed"}};
        } else {
            std::string server_name = r[0]["server_name"].as<std::string>();
            std::string owner = r[0]["owner"].as<std::string>();

            response["server"] = {
                {"server_name", server_name},
                {"owner", owner}
            };
            response["status"] = 200;
        }
    } catch (std::exception &e) {
        response["status"] = "failed";
        std::cout << e.what() << "\n";
    }

    return response;
}

json join_server(const std::string server_id, const std::string UUID) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("INSERT INTO user_servers (sid, uid) VALUES (" + txn.quote(server_id) + ", " + txn.quote(UUID) + ") RETURNING sid;");

        txn.commit();

        std::string sid = r[0]["sid"].as<std::string>();
        response["server"] = {
            {"name", get_server(sid)["server"].value("server_name", "")},
            {"owner", get_server(sid)["server"].value("owner", "")},
            {"serverID", sid},
            {"response", "success"}
        };
    } catch (const pqxx::unique_violation) {
        response["server"] = {
            {"status", 409},
            {"message", "User is already in server"}
        };  
    } catch (std::exception& e) {
        std::cout << e.what() << "\n";
        response["what"] = e.what();
        response["status"] = 404;
    }

    return response;
}

json create_server(const std::string serverName, const std::string UUID) {
    json response;
    boost::uuids::uuid id = boost::uuids::random_generator()();
    std::string server_id = to_string(id);

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec("INSERT INTO servers (server_name, server_id, owner) values (" + txn.quote(serverName) + ", " + txn.quote(server_id) + ", " + txn.quote(UUID) + ") RETURNING *;");
        
        txn.commit();

        if (r.empty()) {
            std::cout << "Server creation failed";
            response["error"] = "Server creation failed";
            response["status"] = 404;
        } else {
            std::string server_name = r[0]["server_name"].as<std::string>();
            std::string server_id = r[0]["server_id"].as<std::string>();
            std::string owner = r[0]["owner"].as<std::string>();
            response["server"] = {
                {"serverName", server_name},
                {"serverID", server_id},
                {"ownerID", owner},
            };
            response["status"] = 200;
        }


    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        response["error"] = e.what();
        response["status"] = 404;
    }

    return response;
}

json update_server() {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(
            ""
        );

    } catch (std::exception& e) {
        std::cout << e.what() << "\n";
    }

    return response;
}