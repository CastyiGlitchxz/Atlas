#include "../../include/database.hpp"
#include "../../include/servers/server_service.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

using json = nlohmann::json;

json server_get_all_users(const std::string_view serverID) {
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
            serverID
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
                {"userID", user_id},
                {"bio", bio}
            });
        };

    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        response["what"] = e.what();
        response["status"] = 404;
    }

    return response;
}

bool delete_server_code(const std::string& code) {
    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string_view query =
            "DELETE from server_invites WHERE code = $1";
        pqxx::result r = txn.exec_params(query, code);
        txn.commit();

        return r.affected_rows() > 0;

    } catch (std::exception &e) {
        std::cerr << "Database error: " << e.what() << "\n";
        return false;
    };
}

json generate_server_code(const std::string& userID, const std::string& sid) {
    json response = json::object();

    try {
        size_t length = 5;
    
        const std::string characters = 
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        
        std::random_device rd;
        std::mt19937 generator(rd());
    
        std::uniform_int_distribution<> distribution(0, characters.length() - 1);
        
        std::string code;
        code.reserve(length);
    
        for (size_t i = 0; i < length; ++i) {
            code += characters[distribution(generator)];
        }

        Database db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "WITH ser_inv AS ("
            " INSERT INTO server_invites (issued_by, code, expires, sid) "
            " VALUES ($1, $2, $3, $4) "
            " RETURNING * "
            ") "
            "SELECT * "
            "FROM ser_inv si "
            "JOIN users u ON si.issued_by = u.user_id";

        // pqxx::result r = txn.exec(
        //     "INSERT INTO server_invites (issued_by, code, expires, sid) VALUES (" +
        //         txn.quote(userID) + "," +
        //         txn.quote(code) + "," +
        //         txn.quote("2027-02-06 00:00:00") + "," +
        //         txn.quote(sid) +
        //     ")"
        // );

        pqxx::result r = txn.exec_params(query, userID, code, "2027-02-06 00:00:00", sid);

        txn.commit();
        std::string displayName = r[0]["displayname"].as<std::string>();
        std::string expires = r[0]["expires"].as<std::string>();

        response["invite_code"] = code;
        response["expires"] = expires;
        response["issued_by"] = {
            {"displayName", displayName},
            {"userID", userID}
        };
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        response = e.what();
    }
    
    return response;
}

json get_all_invite_codes(std::string_view serverID) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string_view query =
            "SELECT si.issued_by, si.code, si.sid, si.expires, u.displayname "
            "FROM server_invites si "
            "JOIN users u ON u.user_id = si.issued_by "
            "WHERE sid = $1";
        pqxx::result r = txn.exec_params(query, serverID);

        if (r.empty()) {
            return response["message"] = "No invite codes";
        } else {

            for (const auto& row : r) {
                std::string displayName = row["displayname"].as<std::string>();
                std::string invite_code = row["code"].as<std::string>();
                std::string expires = row["expires"].as<std::string>();
                std::string userID = row["issued_by"].as<std::string>();
    
                response["codes"].push_back({
                    {"issued_by", {
                        {"displayName", displayName},
                        {"userID", userID}
                    }},
                    {"invite_code", invite_code},
                    {"expires", expires}
                });
                response["status"] = 200;
            }
        }
    } catch (std::exception& e) {
        response["error"] = e.what();
    }

    return response;
}

json verify_invite(const std::string& code) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(
            "SELECT i.issued_by, i.sid "
            "FROM server_invites i "
            "WHERE code = $1",
            code
        );

        if (r.empty()) {
            response["invite"] = {
                {"failed", "server_does_not_exist"}
            };
            response["status"] = 404;
        } else {
            std::string issued_by = r[0]["issued_by"].as<std::string>();
            std::string sid = r[0]["sid"].as<std::string>();
            
            response["invite"] = {
                {"sid", sid},
                {"issued_by", issued_by}
            };
            response["status"] = 200;
        }

        
    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        response["what"] = e.what();
        response["status"] = 404;
    }
    
    return response;
}

json get_server(const std::string_view serverID) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
        std::string_view query =
            "SELECT s.*, u.displayname, u.profile_picture "
            "FROM servers s "
            "JOIN users u ON u.user_id = s.owner "
            "WHERE server_id = $1";

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(query, serverID);

        if (r.empty()) {
            std::cout << "Server not found" << "\n";
            return json{{"status", "failed"}};
        } else {
            std::string serverName = r[0]["server_name"].as<std::string>();
            std::string userID = r[0]["owner"].as<std::string>();
            std::optional<std::string> icon = r[0]["icon"].as<std::optional<std::string>>();

            // remember to come back and fix this by not fetching from the user function but the db itself.
            // Well... past self, I have returned to save this function!
            std::string displayName = r[0]["displayname"].as<std::string>();
            std::string userAvatar = r[0]["profile_picture"].as<std::string>();
            

            response["server"] = {
                {"serverName", serverName},
                {"owner", {
                    {"userID", userID},
                    {"displayName", displayName},
                    {"avatar", userAvatar}
                }},
                {"icon", icon ? json(*icon) : json(nullptr)}
            };
        }
    } catch (std::exception &e) {
        response["status"] = "failed";
        std::cout << e.what() << "\n";
    }

    return response;
}

json check_user_in_server(std::string_view UUID, std::string_view serverID) {
    json response;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT 1 FROM user_servers WHERE uid = $1 AND sid = $2 LIMIT 1", UUID, serverID);

        bool result = !r.empty();
        response = result;

    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        response["what"] = e.what();
        response["result"] = false;
    }

    return response;
}

json join_server(const std::string& serverID, const std::string& userID) {
    json response = json::object();

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec("INSERT INTO user_servers (sid, uid) VALUES (" + txn.quote(serverID) + ", " + txn.quote(userID) + ")");

        txn.commit();

        response["server"] = {
            {"name", get_server(serverID)["server"].value("server_name", "")},
            {"owner", get_server(serverID)["server"]["owner"].value("displayName", "")},
            {"serverID", serverID},
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

json create_server(const std::string_view serverName, const std::string_view UUID) {
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

json remove_user_from_server(const std::string& UUID, const std::string& serverID) {
    json response = json::object();

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string_view query =
        "DELETE from user_servers "
        "WHERE uid = $1 AND sid = $2";

        pqxx::result r = txn.exec_params(query, UUID, serverID);
        txn.commit();

        if (r.empty()) {
            response = "failed";
        } else {
            response = "success";
        }
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        response = "failed";
    }

    return response;
}

json update_server(const json& server, const std::string& serverID) {
    json response = json::object();
    
    try {
        std::cout << "[SERVER] " << server << "\n[serverID]" << serverID << "\n";

        std::string serverName = server.value("serverName", "");

        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        std::string_view query =
            "UPDATE servers "
            "SET server_name = $1 "
            "WHERE server_id = $2";
        pqxx::result r = txn.exec_params(query, serverName, serverID);
        txn.commit();

    } catch (std::exception& e) {
        std::cout << e.what() << "\n";
    }

    return response;
}