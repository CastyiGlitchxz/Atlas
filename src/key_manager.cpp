#include <iostream>
#include <string>
#include <random>
#include <nlohmann/json.hpp>
#include "../include/database.hpp"
#include "../include/key_manager.hpp"

using json = nlohmann::json;

std::variant<bool, KeyMate> validate_apikey(const std::string& key) {
    KeyMate apikey = {};

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
    
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT * FROM apikeys WHERE key = $1", key);
        if (r.empty()) {
            return false;
        } else {
            apikey = {
                .assigner = r[0]["assigner"].as<std::string>(),
                .assignee = r[0]["assignee"].as<std::string>(),
                .key = r[0]["key"].as<std::string>(),
                .clientName = r[0]["client_name"].as<std::string>(),
                .internalSlug = r[0]["internal_slug"].as<std::string>(),
                .isTrusted = r[0]["is_trusted"].as<bool>(),
            };

            return apikey;
        }
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
        return false;
    }
}

std::variant<bool, KeyMate> create_apikey(const std::string& assignee, const std::string& internalSlug, const std::string& clientName) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWSYZ1234567890";
    std::string salted_key;
    KeyMate generated_api_identifier;

    std::random_device rd;
    std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, charset.size() - 1);

    for (int i = 0; i < 36; i++) {
        salted_key += charset[distribution(generator)];
    }

    try {
        bool truthy = std::get<bool>(validate_apikey(salted_key));
        
        if (truthy)
            return false;

        Database db = connect_db();
        auto& conn = db.getConnection();

        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("INSERT INTO apikeys (assigner, assignee, key, internal_slug, is_trusted, client_name) values (" + 
            txn.quote("AOWSS") + "," + 
            txn.quote(assignee) + "," + 
            txn.quote(salted_key) + "," +
            txn.quote(internalSlug) + "," +
            txn.quote(false) + "," +
            txn.quote(clientName) + ") RETURNING *;");
        txn.commit();

        generated_api_identifier.id = r[0]["id"].as<int>();
        generated_api_identifier.assignee = r[0]["assignee"].as<std::string>();
        generated_api_identifier.assigner = r[0]["assigner"].as<std::string>();
        generated_api_identifier.internalSlug = r[0]["internal_slug"].as<std::string>();
        generated_api_identifier.clientName = r[0]["client_name"].as<std::string>();
        generated_api_identifier.isTrusted = r[0]["is_trusted"].as<bool>();
        generated_api_identifier.key = r[0]["key"].as<std::string>();
    
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return false;
    }

    return generated_api_identifier;
}