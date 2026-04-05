#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Function to add a message to the database
json create_message(int room_id, const std::string& user_id, const std::string& content);
