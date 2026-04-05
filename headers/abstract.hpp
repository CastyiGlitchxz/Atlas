#include <string>
#include <optional>

struct User {
    std::string username;
    std::string userid;
    std::string displayName;
    std::string status;
    std::string picture;
    std::string customStatus;
};

struct MessageFormat {
    int id;
    std::string picture;
    std::string displayName;
    std::string serverID;
    std::string content;
    std::string timestamp;
    std::optional<int> messageRef;
    std::optional<std::string> link;
};