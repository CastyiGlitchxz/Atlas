#include <string>
#include <vector>
#include <nlohmann/json.hpp>

void send_push_notification(const std::string& device_token, const nlohmann::json& data);
void send_batch_push_notifications(const std::vector<std::string>& device_tokens, const nlohmann::json& data);
std::vector<std::string> get_server_recipient_tokens(const std::string& serverID, const std::string& deviceToken);