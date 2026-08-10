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

        static char *into_buf(char *begin, char *end, RelationshipTypes const &value) {
            std::string s;
            switch (value) {
                case RelationshipTypes::pending:  s = "pending"; break;
                case RelationshipTypes::accepted: s = "accepted"; break;
                case RelationshipTypes::blocked:  s = "blocked"; break;
                default:                          s = "unknown"; break;
            }
            return string_traits<std::string_view>::into_buf(begin, end, s);
        }

        static size_t size_buffer(RelationshipTypes const &value) noexcept {
            return 32;
        }

        static RelationshipTypes from_string(std::string_view text) {
            if (text == "pending")  return RelationshipTypes::pending;
            if (text == "accepted") return RelationshipTypes::accepted;
            if (text == "blocked")  return RelationshipTypes::blocked;
            throw std::runtime_error("Invalid relationship status in database");
        }

        static constexpr bool is_null(RelationshipTypes const &) noexcept { return false; }
        static constexpr RelationshipTypes null() { return RelationshipTypes::pending; }
    };
}

nlohmann::json send_friend_request(const std::string& Sender_UUID, const std::string& Receiver_UUID);
void change_relationship_status(int requestID, RelationshipTypes status);
void remove_friend(int requestID);
std::optional<nlohmann::json> get_all_friends(const std::string& UUID);
std::optional<nlohmann::json> get_friend_requests(const std::string& UUID);