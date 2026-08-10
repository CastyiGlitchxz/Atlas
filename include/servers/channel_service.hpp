#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include "../models/channel_models.hpp"

nlohmann::json get_server_channels(const std::string& serverID);
nlohmann::json create_server_channel(const ChannelModel& channelModel);
nlohmann::json delete_server_channel(uint64_t channelID);
nlohmann::json update_server_channel(const nlohmann::json& channel);
bool does_channel_exist(uint64_t channelID);