#include "headers/database.hpp"
#include <nlohmann/json.hpp>
#include <argon2.h>
#include <iostream>

using json = nlohmann::json;

json verify_invite(const std::string code) {
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