#include <string>
#include <nlohmann/json.hpp>

struct MessageFormat {
    int id;
    std::string picture;
    std::string displayName;
    uint64_t channelID;
    std::string groupID;
    std::string receiverID;
    std::string content;
    std::string timestamp;
    std::optional<int> messageRef;
    std::optional<std::string> link;
};

nlohmann::json get_messages(uint64_t channelID, std::optional<int> index);
nlohmann::json create_message(const std::string& userID, const MessageFormat& message);
nlohmann::json delete_message(int messageID);
nlohmann::json edit_message(int messageID, std::string& content);