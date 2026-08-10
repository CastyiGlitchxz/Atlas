#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../../include/database.hpp"
#include "../../include/servers/channel_service.hpp"
#include <pqxx/internal/statement_parameters.hxx>
#include <string>
#include <string_view>

using json = nlohmann::json;

json get_server_channels(const std::string& serverID) {
    json payload = json::array();

    try {
        auto db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "SELECT * "
            "FROM channels "
            "WHERE server_id = $1";
        pqxx::result r = txn.exec_params(query, serverID);

        for (const auto& row : r) {
            std::string channelName = row["channel_name"].as<std::string>();
            uint64_t channelID = row["channel_id"].as<uint64_t>();

            payload.push_back({
                {"channelName", channelName},
                {"channelID", std::to_string(channelID)},
            });
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }

    return payload;
}

json create_server_channel(const ChannelModel& channelModel) {
    json payload = json::object();

    try {
        auto db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string query =
            "INSERT INTO channels (channel_name, channel_id, server_id) VALUES (" +
            txn.quote(channelModel.channelName) + ", " +
            txn.quote(channelModel.channelID) + ", " +
            txn.quote(channelModel.serverID) + ") RETURNING *;";
        pqxx::result r = txn.exec(query);
        txn.commit();

        if (r.empty()) {
            payload = "Transaction was not successful";
            std::cout << "Transaction was not successful\n";
            return payload;
        }

        std::string channelName = r[0]["channel_name"].as<std::string>();
        uint64_t channelID = r[0]["channel_id"].as<uint64_t>();

        payload = {
            {"channelName", channelName},
            {"channelID", std::to_string(channelID)}
        };
    } catch (const std::exception& e) {
        payload = e.what();
        std::cerr << "Error: " << e.what() << "\n";
    }

    return payload;
}

json delete_server_channel(uint64_t channelID) {
    json payload = json::object();

    try {
        auto db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "DELETE from channels WHERE channel_id = $1";
        pqxx::result r = txn.exec(query, pqxx::params{
            channelID
        });
        txn.commit();

        if (r.affected_rows() == 0) {
            payload = "Transaction failed";
            return payload;
        }

        payload["channelID"] = std::to_string(channelID);
    } catch (const std::exception& e) {
        payload = e.what();
        std::cerr << "Error: " << e.what() << "\n";
    }

    return payload;
}

json update_server_channel(const json& channel) {
    json payload = json::object();

    try {
        uint64_t channelID = std::stoull(channel.value("channelID", ""));
        std::string channelName = channel.at("channelName");

        auto db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "UPDATE channels SET channel_name = $1 WHERE channel_id = $2";
        pqxx::result r = txn.exec(query, pqxx::params{
            channelName,
            channelID
        });
        txn.commit();
        
        if (r.affected_rows() == 0) {
            payload["error"] = "Transaction not completed";
            return payload;
        }

        payload = channel;
    } catch (const std::exception& e) {
        payload["error"] = e.what();
    }

    return payload;
}

bool does_channel_exist(uint64_t channelID) {
    try {
        auto db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "SELECT EXISTS(SELECT channel_id FROM channels WHERE channel_id = $1)";
        pqxx::result r = txn.exec(query, pqxx::params{channelID});

        return r[0][0].as<bool>();
    } catch (const std::exception& e) {
        return false;
    }

    return false;
}