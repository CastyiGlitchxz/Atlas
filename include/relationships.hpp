#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

enum RelationshipTypes {
    pending,
    accepted,
    blocked
};


namespace pqxx {
    template<> struct string_traits<RelationshipTypes> {
        
        // 1. The modern "to_buf" replacement
        static char *to_buf(char *begin, char *end, RelationshipTypes const &value) {
            std::string_view s;
            switch (value) {
                case RelationshipTypes::pending:  s = "pending"; break;
                case RelationshipTypes::accepted: s = "accepted"; break;
                case RelationshipTypes::blocked:  s = "blocked"; break;
                default:                          s = "unknown"; break;
            }
            
            // Check for buffer overflow
            if (static_cast<std::size_t>(end - begin) <= s.size())
                throw std::runtime_error("libpqxx: conversion buffer too small.");

            std::memcpy(begin, s.data(), s.size());
            begin[s.size()] = '\0';
            return begin + s.size();
        }

        // 2. The size hint (required by to_buf)
        static constexpr std::size_t size_buffer(RelationshipTypes const &) noexcept {
            return 16; // "accepted" + null terminator is 9, so 16 is plenty
        }

        // 3. Conversion from DB string to Enum
        static RelationshipTypes from_string(std::string_view text) {
            if (text == "pending")  return RelationshipTypes::pending;
            if (text == "accepted") return RelationshipTypes::accepted;
            if (text == "blocked")  return RelationshipTypes::blocked;
            throw std::runtime_error("Invalid relationship status in database");
        }

        static constexpr bool is_null(RelationshipTypes) noexcept { return false; }
        static constexpr bool has_null = false;
    };
}

nlohmann::json send_friend_request(const std::string& Sender_UUID, const std::string& Receiver_UUID);
void change_relationship_status(int requestID, RelationshipTypes status);
void remove_friend(int requestID);
std::optional<nlohmann::json> get_all_friends(const std::string& UUID);
std::optional<nlohmann::json> get_friend_requests(const std::string& UUID);