#include <algorithm>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <ostream>
#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include "include/database.hpp"
#include "include/messaging.hpp"
#include <jwt-cpp/jwt.h>
#include "include/file_handler.hpp"
#include "include/models/user_models.hpp"
#include <variant>
#include <yaml-cpp/yaml.h>
#include "include/relationships.hpp"
#include "src/HTTP/HTTPService.cpp"
#include "src/Websocket/WebsocketService.cpp"
#include "include/servers/server_service.hpp"
#include "include/servers/channel_service.hpp"
#include "include/Helpers/TokenHandlers.hpp"
#include "include/Helpers/SM.hpp"
#include "include/notifications/notification_daemon.hpp"
#include "include/Helpers/presence.hpp"
#include "modules/discord/discord_hook.hpp"
#include "modules/discord/server_connection.hpp"

#define allowed_origin "*"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;
using json = nlohmann::json;

inline WebSocketSessionManager g_sessions;
inline PresenceManager g_presence;

//------------------------------------------------------------
// Type alias for HTTP route handlers
//------------------------------------------------------------

using HttpRoute = std::function<http::response<http::string_body>(const http::request<http::string_body>&)>;

//------------------------------------------------------------
// Handle regular HTTP requests
//------------------------------------------------------------
void handle_http(tcp::socket& socket,
                 const http::request<http::string_body>& req,
                 const std::map<std::string, HttpRoute>& routes)
{
    std::string full_target{req.target().data(), req.target().size()};
    size_t query_pos = full_target.find('?');
    
    std::string path = full_target.substr(0, query_pos);

    if (req.method() == http::verb::options) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Boost.Beast");

        res.set(http::field::access_control_allow_origin, allowed_origin);
        res.set(http::field::access_control_allow_credentials, "true");
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type, Authorization, apikey, server-id, page-index, channel-id");
        res.set(http::field::access_control_max_age, "86400"); // Cache preflight result

        // Send the empty response immediately
        http::write(socket, res);
        return;
    }

    auto it = routes.find(path);
    http::response<http::string_body> res;

    if (it != routes.end()) {
        res = it->second(req);
    } else {
        json response_body;
        response_body["error"] = "Endpoint not found";

        res.result(http::status::not_found);
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();
    }

    res.set(http::field::access_control_allow_origin, allowed_origin);
    res.set(http::field::access_control_allow_credentials, "true");

    http::write(socket, res);
}

//------------------------------------------------------------
// Handle WebSocket connections
//------------------------------------------------------------
void handle_websocket(tcp::socket socket, const http::request<http::string_body>& req)
{
    std::shared_ptr<websocket::stream<tcp::socket>> ws_session = nullptr;
    std::string userID = "";

    try {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws_session = ws;

        ws->set_option(websocket::stream_base::timeout{
            std::chrono::seconds(30),
            std::chrono::seconds(15),
            true
        });

        ws->accept(req);
        g_sessions.add(ws);

        std::cout << "[WebSocket] Client connected! Total: "
                  << g_sessions.sessions.size() << "\n";

        std::map<std::string, std::function<json(const json&)>> eventHandlers;

        eventHandlers["send_message"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};

            uint64_t channelID = std::stoull(data.value("channelID", ""));
            std::cout << channelID << "\n";
            if (does_channel_exist(channelID) == false) {
                return json{"error", "Transaction not allowed"};
            }
            
            std::string content = data.value("message", "");
            std::string serverID = data.value("serverID", "");
            std::string token = data.value("token", "");
            std::string device_token = data.value("device_token", "");

            std::string userID = decode_token(token);

            // YO, revamp this code right now!
            json user = get_user_all(userID);
            std::string picture = user.value("picture", "");
            std::string displayName = user.value("displayName", "");

            std::optional<std::string> link = (data.contains("link") && !data["link"].is_null())
            ? std::make_optional(data["link"].get<std::string>())
            : std::nullopt;

            std::optional<int> mRef;

            MessageFormat message {
                .channelID = channelID,
                .content = content,
                .messageRef = mRef,
                .link = link
            };

            json messageObject = create_message(userID, message);

            int messageID = messageObject.at("id");
            std::string time = messageObject.value("timestamp", "");

            if (content.starts_with("[discord] ")) {
                discord_send_message(cenv.find_token("hooks", "webhook_key"), displayName, content, picture);
            }

            json jdata;
            jdata["channelID"] = std::to_string(channelID);
            jdata["displayName"] = displayName;
            jdata["picture"] = picture;
            jdata["content"] = content;
            jdata["id"] = messageID;
            jdata["messageRef"] = mRef ? json(*mRef) : json(nullptr);
            jdata["timestamp"] = time;
            jdata["link"] = link ? json(*link) : json(nullptr);
            jdata["userID"] = userID;

            json msg;
            msg["event"] = "message";
            msg["data"] = jdata;

            std::cout << "[Broadcast] " << content << "\n";

            json notification = {
                {"event", "notification"},
                {"data", {
                    {"sender", {
                        {"userID", userID},
                        {"displayName", displayName},
                        {"message", content},
                        {"picture", picture}
                    }},
                    {"channelID", std::to_string(channelID)},
                    {"serverID", serverID}
                }}
            };

            std::vector<std::string> recipient_tokens = get_server_recipient_tokens(serverID, device_token);

            g_sessions.broadcast(notification);

            if (!recipient_tokens.empty()) {
                for (const auto& tokens : recipient_tokens) {
                    std::cout << "Tokens: " << tokens << "\n";
                }

                send_batch_push_notifications(recipient_tokens, {
                    {"title", "Message from " + displayName},
                    {"body", content},
                    {"serverID", serverID},
                    {"picture", picture},
                });
            }

            g_sessions.broadcast(msg);

            return json{
                {"event", "ack"},
                {"data", {{"message", content}}}
            };
        };

        eventHandlers["delete_message"] = [&](const json& data) {
            json payload = json::object();

            try {
                if (data.empty()) {
                    payload["error"] = "Empty request body";
                    return payload;
                }

                int messageID = data.value("messageID", 0);
                std::string Apikey = data.value("Apikey", "");
                std::string token = data.value("token", "");
                std::string userIDCache = data.value("cachedUID", "");
                std::string userID = decode_token(token);

                auto validation = validate_apikey(Apikey);

                if (!std::holds_alternative<KeyMate>(validation)) {
                    std::cout << "Invalid Api Key" << std::endl;
                    payload["error"] = "Invalid Api Key";
                    return payload;
                }

                if (userID == userIDCache) {
                    delete_message(messageID);
                    payload["event"] = "message_deleted";
                    payload["data"] = {
                        {"success", true},
                        {"id", messageID}
                    };

                    g_sessions.broadcast(payload);
                } else {
                    payload["error"] = "not the same user";
                }
            } catch (std::exception& e) {
                payload["error"] = e.what();
            }

            return payload;
        };

        eventHandlers["edit_message"] = [&](const json& data) {
            int messageID = data.at("messageID");
            std::string content = data.value("content", "");

            edit_message(messageID, content);

            json msg = {
                {"event", "message_edited"},
                {"data", {
                        {"success", true},
                        {"message", "Message edited"},
                        {"id", messageID},
                        {"content", content}
                    }
                }
            };

            std::cout << msg.value("message", "");

            g_sessions.broadcast(msg);

            return json{
                {"event", "ack"},
                {"data", {{"message", "text"}}}
            };
        };

        eventHandlers["reply_to_message"] = [&](const json& data) {
            int messageRef = data.at("refID");
            std::string content = data.value("content", "");
            uint64_t channelID = std::stoull(data.value("channelID", ""));
            std::string token = data.value("token", "");

            std::string userID = decode_token(token);

            std::optional<std::string> link = (data.contains("link") && !data["link"].is_null())
            ? std::make_optional(data["link"].get<std::string>())
            : std::nullopt;

            MessageFormat message {
                .channelID = channelID,
                .content = content,
                .messageRef = messageRef,
                .link = link,
            };

            json message_object = create_message(userID, message);
            int messageID = message_object.at("id");

            json user = get_user_all(userID);
            std::string picture = user.value("picture", "");
            std::string displayName = user.value("displayName", "");
            std::string time = message_object.value("timestamp", "");

            json jdata;
            jdata["channelID"] = channelID;
            jdata["displayName"] = displayName;
            jdata["picture"] = picture;
            jdata["content"] = content;
            jdata["id"] = messageID;
            jdata["messageRef"] = messageRef;
            jdata["timestamp"] = time;
            jdata["link"] = link ? json(*link) : json(nullptr);
            jdata["userID"] = userID;

            json msg;
            msg["event"] = "message";
            msg["data"] = jdata;

            std::cout << msg.value("message", "");

            g_sessions.broadcast(msg);

            return json{
                {"event", "ack"},
                {"data", {{"message", "text"}}}
            };
        };

        eventHandlers["get_invites"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};

            std::string sid = data.value("sid", "");
            json invites = get_all_invite_codes(sid);

            return json{
                {"event", "retreived_invites"},
                {"data", invites}
            };
        };

        eventHandlers["get_user"] = [&](const json& data) {
            std::string token = data.value("token", "");
            std::string user_id = decode_token(token);
            json user = get_user_all(user_id);

            return json{
                {"event", "return_user"},
                {"data", {
                    {"author", {
                        {"userID", user_id},
                        {"displayName", user.value("displayName", "")}
                    }}
                }},
            };
        };

        eventHandlers["leave_server"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            json payload = json::object();

            try {
                if (data.empty()) {
                    payload["error"] = "Empty request body";
                    return payload;
                }
                  
                std::string Apikey = data.value("Apikey", "");
                auto validation = validate_apikey(Apikey);

                if (!std::holds_alternative<KeyMate>(validation)) {
                    std::cout << "Invalid Api Key" << std::endl;
                    payload["error"] = "Invalid Api Key";
                    return payload;
                }

                std::string serverID = data.value("serverID", "");
                std::string token = data.value("token", "");
                std::string userID = decode_token(token);

                json res = remove_user_from_server(userID, serverID);

                payload["event"] = "user_left";
                payload["data"] = {
                    {"userID", userID},
                    {"serverID", serverID}
                };

            } catch (const std::exception& e) {
                std::cout << e.what() << "\n";
                payload["error"] = e.what();
            }

            g_sessions.broadcast(payload);
            return payload;
        };

        eventHandlers["verify_invite"] = [&](const json& data) {
            std::string code = data.value("code", "");
            std::cout << "[Code]: " << code << std::endl;
            json response = verify_invite(code)["invite"];

            if (response.contains("failed")) {
                return json{
                    {"event", "invite"},
                    {"data", {
                        {"failed", "The provided server may or may not exist."},
                    }}
                };
            } else {
                std::string userID = response.value("issued_by", "");
                std::string sid = response.value("sid", "");
                std::string username = get_user_all(userID).value("username", "");
                json server = get_server(sid)["server"];
                std::string serverName = server.value("serverName", "");

                std::optional<std::string> icon = std::nullopt;
                if (server.contains("icon") && !server["icon"].is_null()) {
                    icon = server["icon"].get<std::string>();
                }

                return json{
                    {"event", "invite"},
                    {"data", {
                        {"issued_by", username},
                        {"server", {
                            {"sid", sid},
                            {"server_name", serverName},
                            {"icon", icon},
                        }}
                    }}
                };
            }
        };

        eventHandlers["delete_server_invite"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            std::string code = data.value("code", "");

            bool result = delete_server_code(code);

            if (!result) {
                return json{"failed"};
            }

            return json {
                {"event", "invite_deleted"},
                {"data", code}
            };
        };

        eventHandlers["create_server_invite"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            std::string token = data.value("token", "");
            std::string sid = data.value("sid", "");

            std::string userID = decode_token(token);

            json server_code = generate_server_code(userID, sid);

            return json {
                {"event", "invite_create"},
                {"data", server_code}
            };
        };

        eventHandlers["join_server"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            std::string token = data.value("token", "");
            std::string sid = data.value("sid", "");
            
            std::string userID = decode_token(token);
            
            if (userID.empty()) {
                return json {
                    {"event", "server_response"},
                    {"data", {
                        {"status", "failed"},
                        {"message", "Invalid or expired token."}
                    }}
                };
            }
            
            json sres = join_server(sid, userID);
            
            if (sres.contains("error")) {
                return json {
                    {"event", "server_response"},
                    {"data", {
                        {"status", "failed"},
                        {"message", "Failed to join server, don't ask why."}
                    }}
                };
            }
            
            return json {
                {"event", "server_response"},
                {"data", sres}
            };
        };

        eventHandlers["create_server"] = [&](const json& data) {
            std::string token = data.value("token", "");
            std::string serverName = data.value("server_name", "");

            std::string userID = decode_token(token);
            json server = create_server(serverName, userID);

            return json{
                {"event", "creation_response"},
                {"data", server}
            };

        };

        eventHandlers["accept_friend_request"] = [&](const json& data) {
            try {
                int requestID = data.at("requestID");
                change_relationship_status(requestID, RelationshipTypes::accepted);

                return json{
                    {"event", "request_accepted"},  
                };
            } catch (std::exception& e) {
                std::cout << e.what() << std::endl;

                return json{
                    {"event", "failed_request_acceptance"},
                    {"data", "Failed to accept request"}
                };
            }
        };

        eventHandlers["update_server"] = [&](const json& data) {
            json payload = json::object();

            try {
                json server = data.value("server", json::object());
                std::string serverID = data.value("serverID", "");

                json response = update_server(server, serverID);
                std::cout << server << "\n";
            } catch (const std::exception& e) {
                payload["error"] = e.what();
                std::cout << e.what() << "\n";
            }

            return payload;
        };

        eventHandlers["create_channel"] = [&](const json& data) {
            json payload = json::object();
            payload["event"] = "channel_created";

            try {
                std::string serverID = data.value("serverID", "");
                std::string channelName = data.value("channelName", "");

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<uint64_t> dist(10000000000ULL, 9223372036854775807ULL);

                uint64_t channelID = dist(gen);

                ChannelModel channel = {
                    .serverID = serverID,
                    .channelName = channelName,
                    .channelID = channelID
                };
                payload["data"] = create_server_channel(channel);
            } catch (const std::exception& e) {
                std::cout << e.what() << "\n";
                payload = e.what();
            }

            g_sessions.broadcast(payload);
            return json::object({
                {"ack", ""}
            });
        };

        eventHandlers["delete_channel"] = [&](const json& data) {
            json payload = json::object();
            payload["event"] = "channel_deleted";

            try {
                uint64_t channelID = std::stoull(data.value("channelID", ""));
                if (does_channel_exist(channelID) == false) {
                    return json{"error", "Transaction not allowed"};
                }

                payload["data"] = delete_server_channel(channelID);
            } catch (const std::exception& e) {
                payload = e.what();
            }

            g_sessions.broadcast(payload);
            return json::object();
        };

        eventHandlers["update_channel"] = [&](const json& data) {
            json payload = json::object();
            payload["event"] = "channel_updated";

            try {
                json channel = data.value("channel", json::object());
                json response = update_server_channel(channel);

                std::cout << "Channel Response: " << response << "\n";

                if (response.contains("error")) {
                    payload = "Failed to complete";
                    return payload;
                }

                payload["data"] = response;
            } catch (const std::exception& e) {
                payload["data"] = e.what();
            }

            g_sessions.broadcast(payload);
            return json::object();
        };

        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer);

            if (ws->got_text()) {
                std::string message = beast::buffers_to_string(buffer.data());
                std::cout << "[WebSocket] Received: " << message << "\n";

                json msg = json::parse(message);
                std::string event = msg.value("event", "");
                json data = msg.value("data", json::object());

                if (event == "establish" && msg.contains("token")) {
                    std::string token = msg.value("token", "");
                    userID = decode_token(token);
                    std::cout << "[USERID] " << userID << std::endl;
                    if (userID.empty()) {
                        std::cout << "[WebSocket] Connection rejected: Invalid token." << std::endl;
                        return;
                    }

                    KeyMate apikey = std::get<KeyMate>(validate_apikey(msg.value("apikey", "")));
                    g_presence.set_online(userID, apikey);
                    set_user_appearance_status(userID, "online");

                    json update = {
                        {"event", "update"},
                        {"data", {
                            {"update", {
                                {"status", "online"},
                                {"userID", userID},
                                {"client", {
                                    {"userID", userID},
                                    {"clientName", apikey.clientName},
                                    {"internalSlug", apikey.internalSlug}
                                }}
                            }}
                        }}
                    };

                    g_sessions.broadcast(update);
                } 
                
                else if (event == "upload_profile" && msg.contains("buffer")) {
                    try {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<> dist(0, 9999);

                        int number = dist(gen);

                        size_t decoded_size = boost::beast::detail::base64::decoded_size(msg.value("buffer", "").size());
                        std::vector<unsigned char> buffer(decoded_size);
                        std::string base64_image = msg.value("buffer", "");
                        std::string user_id = msg.value("userid", "");
                        std::string image_name = std::string("user_") + user_id + std::string("_profile_v") + std::to_string(number) + ".webp";

                        if (!base64_image.empty()) {
                            auto result = boost::beast::detail::base64::decode(
                                buffer.data(),
                                base64_image.data(),
                                base64_image.size()
                            );
    
                            std::string path = handle_images(buffer, result, image_name);
                            std::cout << "[Image] " << path << "\n";
    
                            set_user_profile_picture(user_id, path);
    
                            json ack = {{"event", "upload_profile_ack"}, {"data", {{"status", "success"}}}};
                            ws->text(true);
                            ws->write(net::buffer(ack.dump()));
                        }

                    } catch (std::exception& e) {
                        std::cout << e.what() << std::endl;
                    }
                }

                else if (eventHandlers.contains(event)) {
                    json response = eventHandlers[event](data);
                    ws->text(true);
                    ws->write(net::buffer(response.dump()));
                } else {
                    json err = {{"event", "error"}, {"data", {{"message", "Unknown event: " + event}}}};
                    ws->text(true);
                    ws->write(net::buffer(err.dump()));
                }
            }
        }

    } catch (boost::system::system_error const& se) {
        if (se.code() == boost::beast::error::timeout) {
            std::cout << "[WebSocket] Connection closed due to heartbeat timeout.\n";
            g_presence.set_offline(userID);
            set_user_appearance_status(userID, "offline");

            json update = {
                {"event", "update"},
                {"data", {
                    {"update", {
                        {"status", "offline"},
                        {"userID", userID}
                    }}
                }}
            };

            g_sessions.broadcast(update);
        } else {
            std::cout << "[WebSocket] Error: " << se.code().message() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] Error: " << e.what() << "\n";
    }

    if (ws_session) {
        g_sessions.remove(ws_session);
        set_user_appearance_status(userID, "offline");
        json update = {
            {"event", "update"},
            {"data", {
                {"update", {
                    {"status", "offline"},
                    {"userID", userID}
                }}
            }}
        };

        g_sessions.broadcast(update);

        std::cout << "[WebSocket] Session cleaned up. Remaining: " 
                  << g_sessions.sessions.size() << "\n";
    }
}

//------------------------------------------------------------
// Handle a single session (HTTP or WS)
//------------------------------------------------------------
void do_session(tcp::socket socket,
                const std::map<std::string, HttpRoute>& routes)
{
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::string_body> req;

    http::read(socket, buffer, req, ec);

    if (ec) {
        if (ec == http::error::end_of_stream) return;
        std::cerr << "[Session] Read Error: " << ec.message() << "\n";
        return;
    }

    try {
        if (websocket::is_upgrade(req)) {
            handle_websocket(std::move(socket), req);
            return;
        } else {
            handle_http(socket, req, routes);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Session] Logic Error: " << e.what() << "\n";
    }

    if (socket.is_open()) {
        boost::system::error_code error_code;
        error_code = socket.shutdown(tcp::socket::shutdown_both, ec);
 
        if (ec && ec != boost::asio::error::not_connected) {
            std::cerr << "[Session] Shutdown Error: " << ec.message() << "\n";
        }

        error_code = socket.close(ec); 
    }
}

//------------------------------------------------------------
// Main function
//------------------------------------------------------------
int main(int argc, char* argv[]) {
    HTTPManager ht;
    if (argc > 1 && std::string(argv[1]) == "--notify") {
        ping_server(cenv.find_token("hooks", "webhook_key"));
    }

    YAML::Node config = YAML::LoadFile("../config/app-config.yml");

    if (!config["application"] || !config["chat"])
        throw std::runtime_error("Could not find 'application' or 'chat' in 'app-config' file");

    ht.add_route("/api/", [config](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        try {
            std::string apikey = req["Apikey"];
            auto validation = validate_apikey(req["Apikey"]);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {
                res.result(http::status::ok);
                auto client = std::get<KeyMate>(validation);
                std::string env = config["application"]["env"].as<std::string>();
                std::transform(env.begin(), env.end(), env.begin(), [](unsigned char c) {
                    return std::toupper(c);
                });
                response_body["api_version"] =  env + " " + config["application"]["version"].as<std::string>();
                response_body["welcome"] = "Welcome to the Atlas api.";
                response_body["client"] = json::object({
                    {"clientName", client.clientName},
                    {"internalSlug", client.internalSlug},
                    {"apikey", client.key},
                    {"assignee", client.assignee},
                    {"assigner", client.assigner},
                    {"isTrusted", client.isTrusted},
                });
            }
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << "\n";
            response_body["error"] = "Internal server error.";
            response_body["what"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/login", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, allowed_origin);
        res.set(http::field::access_control_allow_credentials, "true");
        json response_body;

        try {
            res.result(http::status::ok);

            auto body = json::parse(req.body());

            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            if (login_user(username, password)) {
                json user = get_user(username);

                // Calculate expiry for 1 week (matches cookie Max-Age)
                auto expiry_time = std::chrono::system_clock::now() + std::chrono::minutes(60 * 24 * 7);

                // 1. Generate JWT token
                auto token = jwt::create()
                    .set_issuer("atlas_scarlet")
                    .set_type("JWT")
                    .set_audience("as-cli")
                    .set_subject(user["user_id"])
                    .set_issued_at(std::chrono::system_clock::now())
                    .set_expires_at(expiry_time)
                    .sign(jwt::algorithm::hs256{secret});

                // std::string cookie_value = "token=" + token +
                //                         "; Path=/; HttpOnly; Max-Age=604800";

                // std::cout << "COOKIE: " << cookie_value;
                // res.set(http::field::set_cookie, cookie_value);

                response_body["response"] = {
                    {"status", 200},
                    {"message", "Login Successful"},
                    {"token", token},
                };

            } else {
                // Login failure (401)
                res.result(http::status::unauthorized);
                response_body["error"] = "Invalid credentials";
            }

        } catch (const std::exception& e) {
            // Error handling (400 Bad Request/500 Internal Server Error)
            res.result(http::status::bad_request);
            response_body["error"] = "Server processing error";
            response_body["details"] = e.what();
        }

        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/logout", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        try {
            res.result(http::status::ok);
            response_body["message"] = "Logout successful.";
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["message"] = e.what();
            std::cout << e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/messages", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;
        
        try {
            uint64_t channelID = std::stoull(req["Channel-ID"]);
            std::string userID = parse_bearer_token(req);
    
            auto validation = validate_apikey(req["Apikey"]);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {    
                res.result(http::status::ok);
                std::optional<int> index;
                std::string_view returned_index = req["Page-Index"];
                std::cout << "[Ret]: " << returned_index << std::endl;

                if (!returned_index.empty()) {
                    try {
                        index = std::stoi(std::string(returned_index));
                    } catch (const std::exception& e) {
                        std::cout << "Index is not a valid integer: " << e.what() << std::endl;
                    }
                }

                if (index.has_value()) {
                    std::cout << "[Value] " << index.value() << std::endl;
                    response_body = get_messages(channelID, index);
                } else {
                    std::cout << "Index is missing or null!\n";
                    response_body = get_messages(channelID, std::nullopt); 
                }
            }
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            response_body["error"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/create", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        try {
            auto body = json::parse(req.body());
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string displayName = body.value("displayName", "");
            std::string bio = body.value("bio", "");
            std::string custom_status = body.value("customStatus", "");

            if (password.size() < 5) {

            }

            create_account(username, displayName, password, custom_status, bio);

            res.result(http::status::ok);

            response_body["status"] = "created";
            response_body["username"] = username;

        } catch (std::exception &e) {
            response_body["error"] = "Invalid JSON";
            response_body["what"] = e.what();
            std::cout << e.what() << "\n";
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    using ImageResponse = http::response<http::vector_body<char>>;

    ht.add_route("/api/account", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::access_control_allow_origin, allowed_origin);
        res.set(http::field::access_control_allow_credentials, "true");

        beast::flat_buffer buffer;
        ImageResponse image_res;
        boost::beast::error_code ec;
        json response_body = json::object();
        
        try {
            std::string userID = parse_bearer_token(req);
            std::string apikey = req["Apikey"];
            res.result(http::status::ok);

            auto validation = validate_apikey(apikey);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {
                if (req.method() == http::verb::post) {
                    if (req.body().empty()) {
                        res.result(http::status::bad_request);
                        response_body["error"] = "Empty request body";
                    } else {
                        auto body = json::parse(req.body());

                        auto user = body["user"];

                        std::string displayname = user.value("displayName", "");
                        std::string username = user.value("username", "");
                        std::string profile_picture = user.value("picture", "");
                        std::string custom_status = user.value("customStatus", "");
                        std::string bio = user.value("bio", "");

                        update_account(username, displayname, profile_picture, custom_status, bio, userID);
                    }
                } else if (req.method() == http::verb::get) {
                    response_body["user"] = get_user_all(userID);
                }
            }
        } catch (const std::runtime_error& e) {
            // --- AUTH FAILURE PATH (e.g., Authorization cookie missing) ---
            // For missing cookie or general auth error, 403 Forbidden is often appropriate
            res.result(http::status::forbidden);
            response_body["error"] = "Authorization failed.";
            response_body["what"] = e.what();
            std::cout << "Auth Error: " << e.what() << "\n";

        } catch (const std::exception& e) {
            // --- JWT FAILURE PATH (e.g., Token expired or invalid signature) ---
            res.result(http::status::unauthorized);
            response_body["error"] = "Invalid or expired token.";
            response_body["what"] = e.what();
            std::cout << "JWT Error: " << e.what() << "\n";

        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/login_status", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::access_control_allow_origin, allowed_origin);
        res.set(http::field::access_control_allow_credentials, "true");

        json response_body;

        try {
            res.result(http::status::ok);
            std::string apikey = req["Apikey"];

            auto validation = validate_apikey(apikey);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {
                std::string userID = parse_bearer_token(req);

                if (userID.empty()) {
                    res.result(http::status::forbidden);
                    response_body["logged_in"] = false;
                } else {
                    response_body["logged_in"] = true;
                }
            }
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["error"] = "Internal server error";
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/users/lookat", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        auto body = json::parse(req.body());
        json response_body;

        try {
            res.result(http::status::ok);
            std::string username = body.value("username", "");

            auto user = lookat_user(username);
            response_body = user;
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/servers", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.result(http::status::ok);
        res.set(http::field::access_control_allow_origin, allowed_origin);

        res.set(http::field::access_control_allow_credentials, "true");
        json response_body;
        
        try {    
            std::string serverID = "";
            std::string userID = parse_bearer_token(req);
            std::string apikey = req["Apikey"];
            
            auto validation = validate_apikey(apikey);
            
            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {
                std::string target(req.target());
                size_t pos = target.find("sid=");
                if (pos != std::string::npos) {
                    serverID = target.substr(pos + 4); 
                    size_t ampersand = serverID.find('&');
                    if (ampersand != std::string::npos) {
                        serverID = serverID.substr(0, ampersand);
                    }
                }

                if (!serverID.empty()) {
                    bool result = check_user_in_server(userID, serverID);

                    if (!result) {
                        res.result(http::status::forbidden);
                        response_body["bad_access_error"] = "User not in server";
                    } else {
                        json server = get_server(serverID);
                        json channels = get_server_channels(serverID);
                        server["server"]["channels"] = channels;

                        response_body = server;
                    }
                } else {
                    response_body = user_get_all_servers(userID);
                }
            }
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["error"] = "Internal Server Error";
            response_body["details"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/servers/userlist", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        try {
            res.result(http::status::ok);
            std::string userID = parse_bearer_token(req);
            std::string serverID = req["Server-ID"];
            std::string apikey = req["Apikey"];

            auto validation = validate_apikey(apikey);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["invalid_apikey_error"] = "Invalid Api Key";
            } else {
                bool result = check_user_in_server(userID, serverID);
                
                if (!result) {
                    res.result(http::status::forbidden);
                    response_body["bad_access_error"] = "User not in server";
                } else {
                    
                    json users_data = server_get_all_users(serverID);
                    
                    if (users_data.contains("user_list") && users_data["user_list"].is_array()) {
                        for (auto& user : users_data["user_list"]) {
                            std::string currentUserID = user["userID"].get<std::string>();
                            
                            auto liveClient = g_presence.get_presence(currentUserID);

                            if (liveClient.has_value()) {
                                user["client"] = {
                                    {"clientName", liveClient->clientName},
                                    {"internalSlug", liveClient->internalSlug},
                                    {"isTrusted", liveClient->isTrusted},
                                };
                            } else {
                                user["client"] = {
                                    {"clientName", nullptr},
                                    {"internalSlug", nullptr},
                                    {"isTrusted", false},
                                };
                            }
                        }
                    }

                    response_body["users"] = users_data;
                }
            }
        } catch (std::exception &e) {
            res.result(http::status::internal_server_error);
            std::cout << e.what() << "\n";
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/friends", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body = json::object();

        try {
            res.result(http::status::ok);
            std::string userID = parse_bearer_token(req);
            std::string serverID = req["Server-ID"];
            std::string apikey = req["Apikey"];

            auto validation = validate_apikey(apikey);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["error"] = "Invalid Api Key";
            } else {
                KeyMate clientMetadata = std::get<KeyMate>(validation);
                json friends = get_all_friends(userID);

                if (friends.contains("friends") && friends["friends"].is_array()) {
                    for (auto& user : friends["friends"]) {
                        std::string currentUserID = user["userID"].get<std::string>();

                        auto liveClient = g_presence.get_presence(currentUserID);

                        if (liveClient.has_value()) {
                            user["client"] = {
                                {"clientName", liveClient->clientName},
                                {"internalSlug", liveClient->internalSlug},
                                {"isTrusted", liveClient->isTrusted},
                            };
                        } else {
                            user["client"] = {
                                {"clientName", nullptr},
                                {"internalSlug", nullptr},
                                {"isTrusted", false},
                            };
                        }
                    }
                }

                response_body = friends;
            }
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["error"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        return res;
    });

    ht.add_route("/api/friends/requests", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body = json::object();

        try {
            res.result(http::status::ok);
            std::string userID = parse_bearer_token(req);
            std::string serverID = req["Server-ID"];
            std::string apikey = req["Apikey"];

            auto validation = validate_apikey(apikey);

            if (!std::holds_alternative<KeyMate>(validation)) {
                res.result(http::status::unauthorized);
                response_body["error"] = "Invalid Api Key";
            } else {
                json requests = get_friend_requests(userID);
                response_body = requests;
            }
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["error"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        return res;
    });

    ht.add_route("/api/createkeys", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        auto body = json::parse(req.body());
        json response_body;

        try {
            res.result(http::status::ok);
            std::string assignee = body.value("assignee", "");
            std::string internalSlug = body.value("internalSlug", "");
            std::string clientName = body.value("clientName", "");

            auto client = std::get<KeyMate>(create_apikey(assignee, internalSlug, clientName));
            response_body["return"] = {
                {"id", client.id},
                {"key", client.key},
                {"assigner", client.assigner},
                {"assignee", client.assignee},
                {"clientName", client.clientName},
                {"internalSlug", client.internalSlug},
                {"isTrusted", client.isTrusted},
            };
        } catch(std::exception& e) {
            std::cout << e.what() << std::endl;
            response_body["error"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    ht.add_route("/api/users/save-token", [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        auto body = json::parse(req.body());
        json response_body;

        try {
            if (body.empty()) {
                res.result(http::status::bad_request);
                response_body["error"] = "Empty request body";
            } else {
                auto validation = validate_apikey(req["Apikey"]);

                if (!std::holds_alternative<KeyMate>(validation)) {
                    res.result(http::status::unauthorized);
                    response_body["invalid_apikey_error"] = "Invalid Api Key";
                } else {
                    std::string device_token = body.value("device_token", "");
                    std::string userID = parse_bearer_token(req);
        
                    if (device_token.empty()) {
                        response_body = "No token provided";
                    } else {
                        std::cout << "Saving token\n";
                        bool res = save_device_token(device_token, userID);

                        if (res == false) {
                            response_body["error"] = "Failed to save device_token";
                        }
                    }
                }
            }
        } catch (std::exception& e) {
            std::cout << e.what() << "\n";
            response_body["error"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    });

    try {
        net::io_context ioc;
        boost::asio::ip::port_type port_number = config["application"]["port"].as<boost::asio::ip::port_type>();
        tcp::acceptor acceptor{ioc, {tcp::v4(), port_number}};
        std::cout << "Server running on:\n  • HTTP → http://localhost:8080/\n  • WS   → ws://localhost:8080/\n";

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(&do_session, std::move(socket), std::cref(ht.routes)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Main] Error: " << e.what() << "\n";
    }
}
