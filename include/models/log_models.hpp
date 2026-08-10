enum LogType {
    user_kicked,
    user_banned,
    message_deleted,
    message_edited,
    server_edited,
    user_timedout
};

struct LogModel {
    LogType type;
};