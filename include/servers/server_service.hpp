#include <nlohmann/json.hpp>
#include <string_view>

nlohmann::json join_server(const std::string& serverID, const std::string& userID);
nlohmann::json get_server(const std::string_view serverID);
nlohmann::json create_server(const std::string_view serverName, const std::string_view UUID);
nlohmann::json check_user_in_server(std::string_view UUID, std::string_view serverID);
nlohmann::json server_get_all_users(const std::string_view serverID);
nlohmann::json remove_user_from_server(const std::string& UUID, const std::string& serverID);
nlohmann::json update_server(const nlohmann::json& server, const std::string& serverID);

bool delete_server_code(const std::string& code);
nlohmann::json generate_server_code(const std::string& userID, const std::string& serverID);
nlohmann::json get_all_invite_codes(const std::string_view serverID);
nlohmann::json verify_invite(const std::string& code);