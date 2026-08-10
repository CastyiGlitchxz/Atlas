#pragma once
#include <iostream>
#include <optional>
#include "../key_manager.hpp"

struct PresenceManager {
    std::mutex mtx;
    std::unordered_map<std::string, KeyMate> active_presence;

    void set_online(const std::string& userID, const KeyMate& client) {
        std::lock_guard<std::mutex> lock(mtx);
        active_presence[userID] = client;
    }

    void set_offline(const std::string& userID) {
        std::lock_guard<std::mutex> lock(mtx);
        active_presence.erase(userID);
    }

    std::optional<KeyMate> get_presence(const std::string& userID) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = active_presence.find(userID);
        if (it != active_presence.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};