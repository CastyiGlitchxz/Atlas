#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include "headers/database.hpp"
#include "headers/messaging.hpp"
#include <jwt-cpp/jwt.h>
#include <vector>
#include "headers/cenv.hpp"
#include "headers/abstract.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
// namespace asio = boost::asio;

namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;
using json = nlohmann::json;

cenvxx clangxx;
auto cenv = clangxx.init("../secrets/cenv");

std::string secret = cenv.find_token("secrets", "securekey");

// -------------------------
// Global session manager
// -------------------------
struct WebSocketSessionManager {
    std::mutex mtx;
    std::vector<std::shared_ptr<websocket::stream<tcp::socket>>> sessions;

    void add(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        std::lock_guard<std::mutex> lock(mtx);
        sessions.push_back(ws);
    }

    void remove(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        std::lock_guard<std::mutex> lock(mtx);
        sessions.erase(std::remove(sessions.begin(), sessions.end(), ws), sessions.end());
    }

    void broadcast(const json& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& s : sessions) {
            try {
                s->text(true);
                s->write(net::buffer(msg.dump()));
            } catch (...) {
                // Ignore failed sends
            }
        }
    }
};

inline WebSocketSessionManager g_sessions; // 🔥 define globally before functions

//------------------------------------------------------------
// Type alias for HTTP route handlers
//------------------------------------------------------------

// Change this:
// using HttpRoute = std::function<json(const http::request<http::string_body>&)>;

// TO THIS:
using HttpRoute = std::function<http::response<http::string_body>(const http::request<http::string_body>&)>;

//------------------------------------------------------------
// Helper to send HTTP JSON response (with added CORS headers)
//------------------------------------------------------------
void send_json_response(tcp::socket& socket,
                        const http::request<http::string_body>& req,
                        const json& j,
                        http::status status = http::status::ok)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "Boost.Beast");
    res.set(http::field::content_type, "application/json");

    // 🔥🔥 ADD THESE CORS HEADERS 🔥🔥
    res.set(http::field::access_control_allow_origin, "http://localhost:3000"); // Use the specific origin or "*"
    res.set(http::field::access_control_allow_credentials, "true");
    res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type, Authorization"); // Ensure Authorization is included if you use it!

    res.body() = j.dump();
    res.prepare_payload();
    http::write(socket, res);
}

//------------------------------------------------------------
// Handle regular HTTP requests
//------------------------------------------------------------
// The NEW handle_http:

//------------------------------------------------------------
// Handle regular HTTP requests (The FINAL, working version)
//------------------------------------------------------------
void handle_http(tcp::socket& socket,
                 const http::request<http::string_body>& req,
                 const std::map<std::string, HttpRoute>& routes)
{
    // 1. Get the requested path
    std::string path{req.target().data(), req.target().size()};

    // 🔥 CATCH THE OPTIONS (PREFLIGHT) REQUEST FIRST 🔥
    if (req.method() == http::verb::options) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Boost.Beast");

        // The browser is hitting 100.95.199.94:8080 from localhost:3000
        res.set(http::field::access_control_allow_origin, "*"); 
        res.set(http::field::access_control_allow_credentials, "true");
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type, Authorization"); 
        res.set(http::field::access_control_max_age, "86400"); // Cache preflight result

        // Send the empty response immediately
        http::write(socket, res);
        return; 
    }
    
    // 2. Handle Actual Request (GET, POST, etc.)
    auto it = routes.find(path);
    http::response<http::string_body> res; 

    if (it != routes.end()) {
        // Route handler returns the final response object (with cookies/body)
        res = it->second(req); 
    } else {
        // Handle 404 Not Found 
        json response_body;
        response_body["error"] = "Endpoint not found";
        
        res.result(http::status::not_found);
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();
    }

    res.set(http::field::access_control_allow_origin, "*"); 
    res.set(http::field::access_control_allow_credentials, "true");
    
    // Write the resulting response
    http::write(socket, res);
}

std::string decode_token(const std::string& token) {
    auto decoded = jwt::decode(token);
    jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret}).verify(decoded);
    
    return decoded.get_subject();
};

std::string set_user_appearance_status(const std::string& UUID, const std::string& status);
json create_message(const std::string& user_id, const MessageFormat& message);
json delete_message(int message_id);
json edit_message(int message_id, std::string& content);
json get_user_all(const std::string& UUID);
json verify_invite(const std::string code);
json join_server(const std::string server_id, const std::string UUID);
json get_server(const std::string server_id);
json create_server(const std::string serverName, const std::string UUID);

// Helper function (outside the handler)
std::string get_user_id_from_cookie(const http::request<http::string_body>& req) {
    if (!req.count(http::field::cookie)) {
        throw std::runtime_error("Authorization cookie missing.");
    }

    std::string cookie_header = req[http::field::cookie];
    // 💡 In production, you would use a robust parser.
    // For simplicity, we'll manually find the token string:
    std::string token;
    size_t start = cookie_header.find("token=");
    if (start != std::string::npos) {
        start += 6; // Length of "token="
        size_t end = cookie_header.find(";", start);
        if (end == std::string::npos) {
            token = cookie_header.substr(start); // Token is at end of string
        } else {
            token = cookie_header.substr(start, end - start);
        }
    } else {
        throw std::runtime_error("Auth token not found in cookie header.");
    }

    return decode_token(token);
}

std::string parse_bearer_token(const http::request<http::string_body>& req) {
    if (!req.count(http::field::authorization)) {
        throw std::runtime_error("[SYSTEM] Authorization bearer token is missing");
    }

    std::string token;
    std::string bearer_token = req[http::field::authorization];
    size_t start = bearer_token.find("Bearer ");

    if (start != std::string::npos) {
        start+=7;
        token = bearer_token.substr(start);
    } else {
        throw std::runtime_error(token + " | Something has happened");
    }

    auto decoded = jwt::decode(token);
    jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret}).verify(decoded);
    
    // Return the subject (user ID)
    return decoded.get_subject();
}

void handle_images(auto ws, beast::flat_buffer buffer, const std::string image_name) {
    std::cout << "[WebSocket] Received binary data of size: " << buffer.size() << "\n";
    std::string base = "../uploads/users/photos/";
    std::string fpath = base + image_name;

    std::string message = beast::buffers_to_string(buffer.data());

    // Save the uploaded file
    std::ofstream out(fpath, std::ios::binary);
    out.write(static_cast<const char*>(buffer.data().data()), buffer.size());
    out.close();

    // Send acknowledgment back to client
    json ack = {{"event", "upload_profile_ack"}, {"data", {{"status", "success"}}}};
    ws->text(true);
    ws->write(net::buffer(ack.dump()));

    std::cout << "[Server] Profile image saved!\n";
}

int discord_sendM(const std::string username, const std::string message) {        
    try {
        const std::string host = "discord.com";
        const std::string port = "443";
        std::string target = cenv.find_token("hooks", "webhook_key");
            
        int version = 11; // HTTP/1.1

        // The JSON payload
        // std::string body = R"({"content": "})";
        // std::string body = std::format(R"({"content": "{}"})", message);
        std::string body = "{\"content\": \"" + message + "\\n\\nSent from Atlas Scarlet" + "\", \"username\": \"" + username + "\"}";

        net::io_context ioc;
        ssl::context ctx{ssl::context::sslv23_client};

        // Resolver
        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve(host, port);

        // Stream
        ssl::stream<tcp::socket> stream{ioc, ctx};

        // Connect
        net::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        // Set up the HTTP POST request
        http::request<http::string_body> req{http::verb::post, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        // Send the request
        http::write(stream, req);

        // Get the response
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        std::cout << res << std::endl;

        // Graceful shutdown
        beast::error_code ec;
        void(stream.shutdown(ec));
        if(ec == net::error::eof) ec = {}; // Ignore EOF
        if(ec) throw beast::system_error{ec};

    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

//------------------------------------------------------------
// Handle WebSocket connections
//------------------------------------------------------------
void handle_websocket(tcp::socket socket, const http::request<http::string_body>& req)
{
    try {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws->accept(req);
        g_sessions.add(ws);

        std::cout << "[WebSocket] Client connected! Total: "
                  << g_sessions.sessions.size() << "\n";

        std::map<std::string, std::function<json(const json&)>> eventHandlers;

        eventHandlers["send_message"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            
            std::string content = data.value("message", "");
            std::string sid = data.value("sid", "");
            std::string token = data.value("token", "");

            std::string user_id = decode_token(token);
            
            json user = get_user_all(user_id);
            std::string picture = user.value("picture", "");
            std::string displayName = user.value("displayName", "");

            std::optional<std::string> link = (data.contains("link") && !data["link"].is_null()) 
            ? std::make_optional(data["link"].get<std::string>()) 
            : std::nullopt;
            
            std::optional<int> mRef;
            
            MessageFormat message {
                .serverID = sid,
                .content = content,
                .messageRef = mRef,
                .link = link
            };

            json message_object = create_message(user_id, message);
            std::string message_id = message_object.value("id", "");
            std::string time = message_object.value("timestamp", "");

            // discord_sendM(displayname, text);

            json jdata;
            jdata["serverID"] = sid;
            jdata["displayName"] = displayName;
            jdata["picture"] = picture;
            jdata["content"] = content;
            jdata["id"] = std::stoi(message_id);
            jdata["messageRef"] = mRef ? json(*mRef) : json(nullptr);
            jdata["timestamp"] = time;
            jdata["link"] = link ? json(*link) : json(nullptr);

            json msg;
            msg["event"] = "message";
            msg["data"] = jdata;


            std::cout << "[Broadcast] " << content << "\n";

            g_sessions.broadcast(msg); // 🔥 broadcast to everyone


            // Respond back to sender as acknowledgment
            return json{
                {"event", "ack"},
                {"data", {{"message", content}}}
            };
        };

        eventHandlers["delete_message"] = [&](const json& data) {
            std::string message_id = data.value("message_id", "");

            delete_message(std::stoi(message_id));

            json msg = {
                {"event", "message_deleted"},
                {"data", {
                        {"success", true},
                        {"message", "Message deleted from chat"},
                        {"id", std::stoi(message_id)}
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

        eventHandlers["edit_message"] = [&](const json& data) {
            std::string message_id = data.value("message_id", "");
            std::string content = data.value("content", "");

            edit_message(std::stoi(message_id), content);

            json msg = {
                {"event", "message_edited"},
                {"data", {
                        {"success", true},
                        {"message", "Message edited"},
                        {"id", std::stoi(message_id)},
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
            std::string messageRef = data.value("ref_id", "");
            std::string content = data.value("content", "");
            std::string sid = data.value("sid", "");
            std::string token = data.value("token", "");
            
            std::string user_id = decode_token(token);

            std::optional<std::string> link = (data.contains("link") && !data["link"].is_null()) 
            ? std::make_optional(data["link"].get<std::string>()) 
            : std::nullopt;

            MessageFormat message {
                .serverID = sid,
                .content = content,
                .messageRef = std::stoi(messageRef),
                .link = link,
            };

            json message_object = create_message(user_id, message);
            std::string message_id = message_object.value("id", "");

            json user = get_user_all(user_id);
            std::string picture = user.value("picture", "");
            std::string displayName = user.value("displayName", "");
            std::string time = message_object.value("timestamp", "");

            json jdata;
            jdata["serverID"] = sid;
            jdata["displayName"] = displayName;
            jdata["picture"] = picture;
            jdata["content"] = content;
            jdata["id"] = std::stoi(message_id);
            jdata["messageRef"] = messageRef;
            jdata["timestamp"] = time;
            jdata["link"] = link ? json(*link) : json(nullptr);

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

        eventHandlers["ping"] = [&](const json&) {
            return json{{"event", "pong"}, {"data", {{"time", time(nullptr)}}}};
        };

        eventHandlers["schedule_notification"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};

            std::string content = data.value("content", "");
            std::string token = data.value("token", "");
            std::string serverID = data.value("sid", "");

            std::string user_id = decode_token(token);

            json user = get_user_all(user_id);

            std::string displayname = user.value("displayName", "");
            std::string pfp = user.value("picture", "");
            std::string username = user.value("username", "");

            json notification = {
                {"event", "notification"},
                {"data", {
                    {"sender", {
                        {"token", user_id},
                        {"displayName", displayname},
                        {"message", content},
                        {"picture", pfp}
                    }},
                    {"serverID", serverID}
                }}
            };

            g_sessions.broadcast(notification);

            return json{
                {"event", "ack"},
                {"data", {{"message", "text"}}}
            };
        };

        eventHandlers["upload_profile"] = [&](const json& data) {
            std::cout << data << "e";

            return json{{"event", "pong"}, {"data", {{"time", time(nullptr)}}}};
        };

        eventHandlers["get_user"] = [&](const json& data) {
            std::string token = data.value("token", "");
            std::string user_id = decode_token(token);
            json user = get_user_all(user_id);
 
            return json{
                {"event", "return_user"},
                {"data", user},
            };
        };

        eventHandlers["update_status"] = [&](const json& data) {
            std::string token = data.value("auth", "");
            std::string status = data.value("status", "");;
            
            std::string user_id = decode_token(token);

            std::string update_type = set_user_appearance_status(user_id, status);

            json update = {
                {"event", "update"},
                {"data", {
                    {"update", {
                        {"status", update_type},
                        {"userID", user_id}
                    }}
                }}
            };

            g_sessions.broadcast(update);

            return json{
                {"event", "ack"},
                {"data", {{"message", "text"}}}
            };
        };

        eventHandlers["verify_invite"] = [&](const json& data) {
            std::string code = data.value("code", "");
            json response = verify_invite(code)["invite"];

            if (response.contains("failed")) {
                return json{
                    {"event", "invite"},
                    {"data", {
                        {"failed", "The provided server may or may not exist."},
                    }}
                };
            } else {
                std::string user_id = response.value("issued_by", "");
                std::string sid = response.value("sid", "");
                std::string username = get_user_all(user_id).value("username", "");
                std::string server_name = get_server(sid)["server"].value("server_name", "");
    
                return json{
                    {"event", "invite"},
                    {"data", {
                        {"issued_by", username},
                        {"server", {
                            {"sid", sid},
                            {"server_name", server_name},
                        }}
                    }}
                };
            }
        };

        eventHandlers["join_server"] = [&](const json& data) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            std::string token = data.value("token", "");

            std::string user_id = decode_token(token);
            std::string sid = data.value("sid", "");

            json sres = join_server(sid, user_id);

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
            std::string token = data.value("auth", "");
            std::string serverName = data.value("server_name", "");

            auto decoded = jwt::decode(token);
            jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret}).verify(decoded);
            
            std::string user_id = decoded.get_subject();
            json server = create_server(serverName, user_id);

            return json{
                {"event", "creation_response"},
                {"data", server}
            };

        };

        // Main receive loop
        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer);

            if (ws->got_text()) {
                // JSON text message
                std::string message = beast::buffers_to_string(buffer.data());
                std::cout << "[WebSocket] Received: " << message << "\n";

                json msg = json::parse(message);
                std::string event = msg.value("event", "");
                json data = msg.value("data", json::object());

                if (eventHandlers.contains(event)) {
                    json response = eventHandlers[event](data);
                    ws->text(true);
                    ws->write(net::buffer(response.dump()));
                } else {
                    json err = {{"event", "error"}, {"data", {{"message", "Unknown event: " + event}}}};
                    ws->text(true);
                    ws->write(net::buffer(err.dump()));
                }
            } else {
                handle_images(ws, buffer, "profile.jpg");
                // Binary data received → treat as profile image
                // std::cout << "[WebSocket] Received binary data of size: " << buffer.size() << "\n";
                // std::string base = "../uploads/users/photos/";
                // std::string fname;

                // std::string message = beast::buffers_to_string(buffer.data());

                // // Save the uploaded file
                // std::ofstream out("../uploads/users/photos/profile.jpg", std::ios::binary);
                // out.write(static_cast<const char*>(buffer.data().data()), buffer.size());
                // out.close();

                // // Send acknowledgment back to client
                // json ack = {{"event", "upload_profile_ack"}, {"data", {{"status", "success"}}}};
                // ws->text(true);
                // ws->write(net::buffer(ack.dump()));

                // std::cout << "[Server] Profile image saved!\n";
            }
        }


    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] Error: " << e.what() << "\n";
    }
}

//------------------------------------------------------------
// Handle a single session (HTTP or WS)
//------------------------------------------------------------
void do_session(tcp::socket socket,
                const std::map<std::string, HttpRoute>& routes)
{
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        if (websocket::is_upgrade(req)) {
            handle_websocket(std::move(socket), req);
        } else {
            handle_http(socket, req, routes);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Session] Error: " << e.what() << "\n";
    }
}

json get_user(const std::string& username);

void create_account(const std::string& username, const std::string& displayName, const std::string& password, const std::string& custom_status, const std::string& bio);
bool login_user(std::string& username, std::string& password);
json get_messages(const std::string serverID);
void update_account(const std::string& username, const std::string& displayname, const std::string& profile_picture, const std::string& custom_status, const std::string& bio, const std::string& UUID);
json user_get_all_servers(const std::string& UUID);

int ping_server() {        
    try {
        const std::string host = "discord.com";
        const std::string port = "443";
        std::string target = cenv.find_token("hooks", "webhook_key");
            
        int version = 11; // HTTP/1.1

        // The JSON payload
        std::string body = R"({"content": "Atlas Server is active."})";

        net::io_context ioc;
        ssl::context ctx{ssl::context::sslv23_client};

        // Resolver
        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve(host, port);

        // Stream
        ssl::stream<tcp::socket> stream{ioc, ctx};

        // Connect
        net::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        // Set up the HTTP POST request
        http::request<http::string_body> req{http::verb::post, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        // Send the request
        http::write(stream, req);

        // Get the response
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        std::cout << res << std::endl;

        // Graceful shutdown
        beast::error_code ec;
        void(stream.shutdown(ec));
        if(ec == net::error::eof) ec = {}; // Ignore EOF
        if(ec) throw beast::system_error{ec};

    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

json server_get_all_users(const std::string server_id);

//------------------------------------------------------------
// Main function
//------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--notify") {
        ping_server();
    }

    std::map<std::string, HttpRoute> routes;

    // Root endpoint
    // routes["/"] = [](const http::request<http::string_body>& req) {
    //     http::response<http::string_body> res{http::status::ok, req.version()};
    //     json response_body;
    //     auto body = json::parse(req.body());

    //     try {
    //         res.result(http::status::ok); 

    //     } catch (const std::exception &e) {
    //         std::cout << "Error: " << e.what() << "\n";
    //         response_body["error"] = "Internal server error.";
    //         response_body["what"] = e.what();
    //     }

    //     res.set(http::field::content_type, "application/json");
    //     res.body() = response_body.dump();
    //     res.prepare_payload();

    //     return res;
    // };

    // Login endpoint
    // Within your main function, replacing the current routes["/login"] definition:

    routes["/api/login"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "application/json");

        // 🔥 Always include CORS headers here for all paths.
        res.set(http::field::access_control_allow_origin, "http://localhost:3000"); 
        res.set(http::field::access_control_allow_credentials, "true");

        // Set other necessary CORS preflight headers if the client is sending an OPTIONS request
        // (though handle_http should cover this, it's safer to ensure they're here too if needed).
        // ...

        try {
            res.result(http::status::ok);

            json response_body;
            auto body = json::parse(req.body());

            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            json user = get_user(username); // Assumes this gets user details, including user_id

            if (login_user(username, password)) {
                // Calculate expiry for 1 week (matches cookie Max-Age)
                auto expiry_time = std::chrono::system_clock::now() + std::chrono::minutes(60 * 24 * 7);

                // 1. Generate JWT token
                auto token = jwt::create()
                    .set_issuer("atlas_scarlet")
                    .set_type("JWT")
                    .set_audience("as-cli")
                    .set_subject(user["user_id"])
                    .set_issued_at(std::chrono::system_clock::now())
                    .set_expires_at(expiry_time) // 🔥 FIXED: 1 week expiry (matches cookie max-age)
                    .sign(jwt::algorithm::hs256{secret}); 

                // std::string cookie_value = "token=" + token + 
                //                         "; Path=/; HttpOnly; Max-Age=604800";
                
                // std::cout << "COOKIE: " << cookie_value;
                // res.set(http::field::set_cookie, cookie_value);

                // 3. Set the JSON body for success
                response_body["response"] = {
                    {"status", 200},
                    {"message", "Login Successful"},
                    {"token", token},
                };
                res.body() = response_body.dump();
                res.prepare_payload();

            } else {
                // Login failure (401)
                res.result(http::status::unauthorized);
                response_body["error"] = "Invalid credentials";
                res.body() = response_body.dump();
                res.prepare_payload();
            }

        } catch (const std::exception& e) {
            // Error handling (400 Bad Request/500 Internal Server Error)
            res.result(http::status::bad_request);
            json response_body;
            response_body["error"] = "Server processing error";
            response_body["details"] = e.what();
            res.body() = response_body.dump();
            res.prepare_payload();
        }

        return res;
    };

    routes["/api/logout"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;
        auto body = json::parse(req.body());

        try {
            res.result(http::status::ok);
            response_body["status"] = 200;
            response_body["message"] = "Logout successful.";
        } catch (std::exception& e) {
            res.result(http::status::internal_server_error);
            response_body["status"] = 500;
            response_body["message"] = e.what();
            std::cout << e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    routes["/api/messages_get"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        res.set(http::field::access_control_allow_origin, "http://localhost:3000"); 
        res.set(http::field::access_control_allow_credentials, "true");

        try {
            std::string body_str = req.body();

            if (body_str.empty()) {
                // return json{{"error", "Empty body received"}};
                response_body["error"] = {
                    {"msg", "server error"}
                };
            }
            
            auto body_json = json::parse(body_str);
            std::string serverID = body_json["sid"];
            
            res.result(http::status::ok); 

            auto messages = get_messages(serverID);
            response_body["messages"] = messages;
            response_body["status"] = 200;
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << "\n";
            response_body["error"] = "Internal server error.";
            response_body["what"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    routes["/api/create"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        try {
            auto body = json::parse(req.body());
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string displayName = body.value("displayName", "");
            std::string bio = body.value("bio", "");
            std::string custom_status = body.value("customStatus", "");
    
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
    };

    routes["/api/account/get"] = [](const http::request<http::string_body>& req) {
        // 1. Declare the response object with a default state (e.g., 401)
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        auto body = json::parse(req.body());
        std::string user_id = parse_bearer_token(req);

        // 🔥 Always include CORS headers before returning any response
        res.set(http::field::access_control_allow_origin, "http://localhost:3000"); 
        res.set(http::field::access_control_allow_credentials, "true");

        try {

            res.result(http::status::ok); 

            json user = get_user_all(user_id);

            response_body["status"] = 200;
            response_body["user"] = user;

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

        // 2. Finalize Response: Set content type, body, and payload
        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    routes["/api/login_status"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        // 🔥 Always include CORS headers before returning any response
        res.set(http::field::access_control_allow_origin, "http://localhost:3000"); 
        res.set(http::field::access_control_allow_credentials, "true");

        try {
            res.result(http::status::ok);

            if (parse_bearer_token(req).empty()) {     
                response_body["status"] = 200;
                response_body["logged_in"] = false;
            }

            res.result(http::status::ok);

            response_body["status"] = 200;
            response_body["logged_in"] = true;
        } catch (...) {
            response_body["status"] = 404;
            response_body["logged_in"] = false;
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    using ImageResponse = http::response<http::vector_body<char>>;

    routes["/api/account/update"] = [](const http::request<http::string_body>& req) {
        beast::flat_buffer buffer;
        ImageResponse image_res;
        boost::beast::error_code ec;
        
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;

        std::string user_id = parse_bearer_token(req);

        res.set(http::field::access_control_allow_origin, "http://localhost:3000"); 
        res.set(http::field::access_control_allow_credentials, "true");

        try {
            if (req.body().empty()) {
                throw std::runtime_error("Empty body");
            }
            auto body = json::parse(req.body());

            auto user = body["user"];

            std::string displayname = user.value("displayName", "");
            std::string username = user.value("username", "");
            std::string profile_picture = user.value("picture", "");
            std::string custom_status = user.value("customStatus", "");
            std::string bio = user.value("bio", "");

            update_account(username, displayname, profile_picture, custom_status, bio, user_id);

            response_body["status"] = 200;
        } catch (std::exception &e) {
            response_body["error"] = "Invalid JSON";
            response_body["what"] = e.what();
            std::cout << e.what() << "\n";
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    routes["/api/servers/get"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        json response_body;
        auto body = json::parse(req.body());
        std::string user_id = parse_bearer_token(req);

        res.set(http::field::access_control_allow_origin, "http:://localhost:3000");
        res.set(http::field::access_control_allow_credentials, "true");

        try {
            // std::string user_id = get_user_id_from_cookie(req);

            res.result(http::status::ok);

            json servers = user_get_all_servers(user_id);

            response_body["servers"] = servers;
            response_body["status"] = 200;

        } catch (std::exception &e) {
            std::cout << e.what() << "\n";
            response_body["error"] = 404;
            response_body["what"] = e.what();
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    routes["/api/servers/userlist_get"] = [](const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        auto body = json::parse(req.body());
        json response_body;

        try {
            if (req.body().empty()) {
                throw std::runtime_error("Empty body");
            }
            std::string server_id = body.value("serverID", "");

            res.result(http::status::ok);

            json user = server_get_all_users(server_id);

            response_body["users"] = user;

        } catch (std::exception &e) {
            std::cout << e.what() << "\n";
        }

        res.set(http::field::content_type, "application/json");
        res.body() = response_body.dump();
        res.prepare_payload();

        return res;
    };

    try {
        net::io_context ioc;
        tcp::acceptor acceptor{ioc, {tcp::v4(), 8080}};
        std::cout << "Server running on:\n  • HTTP → http://localhost:8080/\n  • WS   → ws://localhost:8080/\n";

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(&do_session, std::move(socket), std::cref(routes)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Main] Error: " << e.what() << "\n";
    }
}