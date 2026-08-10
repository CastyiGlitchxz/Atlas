#include <iostream>
#include <variant>

struct KeyMate {
    size_t id;
    std::string assigner;
    std::string assignee;
    std::string key;
    std::string clientName;
    std::string internalSlug;
    bool isTrusted;
};

std::variant<bool, KeyMate> validate_apikey(const std::string& key);
std::variant<bool, KeyMate> create_apikey(const std::string& assignee, const std::string& slug, const std::string& clientName);