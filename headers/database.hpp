#include <pqxx/pqxx>
#include "cenv.hpp"

class Database {
public:
    Database(const std::string& conn_str);
    pqxx::connection& getConnection();

private:
    pqxx::connection conn;  // <-- plain object, not a reference
};

inline Database connect_db() {
    cenvxx clangxx;
    auto cenv = clangxx.init("../secrets/cenv");

    std::string dbname = cenv.find_token("database", "dbname");
    std::string user = cenv.find_token("database", "user");
    std::string password = cenv.find_token("database", "password");
    std::string host = cenv.find_token("database", "host");

    std::string conn_str = "dbname=" + dbname + " user=" + user + " password=" + password + " host=" + host;
    Database db(conn_str);

    return db;
}