#include <pqxx/pqxx>
#include "cenv.hpp"
#include <nlohmann/json.hpp>

class Database {
public:
    Database(const std::string& conn_str);
    pqxx::connection& getConnection();

private:
    pqxx::connection conn;
};

inline Database connect_db() {
    cenvxx clangxx;
    auto cenv = clangxx.init("../config/cenv");

    std::string dbname = cenv.find_token("database", "dbname");
    std::string user = cenv.find_token("database", "user");
    std::string password = cenv.find_token("database", "password");
    std::string host = cenv.find_token("database", "host");

    std::string conn_str = "dbname=" + dbname + " user=" + user + " password=" + password + " host=" + host;
    Database db(conn_str);

    return db;
}

nlohmann::json user_get_all_servers(const std::string& UUID);
void create_account(const std::string& username, const std::string& displayName, const std::string& password, const std::string& custom_status, const std::string& bio);
bool login_user(std::string& username, std::string& password);
void update_account(const std::string& username, const std::string& displayname, const std::string& profile_picture, const std::string& custom_status, const std::string& bio, const std::string& UUID);
nlohmann::json get_user(const std::string& username);
std::string set_user_appearance_status(const std::string& UUID, const std::string& status);
nlohmann::json get_user_all(const std::string& UUID);
nlohmann::json lookat_user(std::string& username);
std::string set_user_profile_picture(const std::string& UUID, const std::string& avatarURL);
bool save_device_token(const std::string& device_token, const std::string& userID);