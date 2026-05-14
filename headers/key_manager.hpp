#include <iostream>
#include <variant>

struct KeyMate {
    size_t id;
    std::string assigner;
    std::string assignee;
    std::string key;
};

bool validate_apikey(const std::string& key);
std::variant<bool, KeyMate> create_apikey(const std::string& assignee);