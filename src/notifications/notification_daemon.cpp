#include <iostream>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl.hpp>
#include <exception>
#include <string>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <vector>
#include "../../include/notifications/notification_daemon.hpp"
#include "../../include/database.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;

using json = nlohmann::json;

// void send_push_notification(const std::string& device_token, const json& data) {
//     json payload = json::object();

//     try {
//         const std::string host = "exp.host";
//         const std::string port = "443";
//         const std::string target = "/--/api/v2/push/send";

//         int version = 11;

//         payload = {
//             {"to", device_token},
//             {"body", body},
//             {"data", data},
//             {"title", title}
//         };

//         if (payload.empty()) return;

//         net::io_context ioc;
//         ssl::context ctx{ssl::context::sslv23_client};

//         tcp::resolver resolver{ioc};
//         auto const results = resolver.resolve(host, port);

//         ssl::stream<tcp::socket> stream{ioc, ctx};

//         net::connect(stream.next_layer(), results.begin(), results.end());
//         stream.handshake(ssl::stream_base::client);

//         http::request<http::string_body> req{http::verb::post, target, version};
//         req.set(http::field::host, host);
//         req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
//         req.set(http::field::content_type, "application/json");
//         req.body() = payload;
//         req.prepare_payload();

//         http::write(stream, req);

//         beast::flat_buffer buffer;
//         http::response<http::string_body> res;
//         http::read(stream, buffer, res);

//         std::cout << res << "\n";

//         beast::error_code ec;
//         void(stream.shutdown(ec));
//         if (ec == net::error::eof) ec ={};
//         if (ec) throw beast::system_error{ec};
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//     }

//     return;
// }

void send_batch_push_notifications(const std::vector<std::string>& device_tokens, const json& data) {
    json payload = json::array();
    std::unordered_set<std::string> processed_tokens;

    try {
        const std::string host = "exp.host";
        const std::string port = "443";
        const std::string target = "/--/api/v2/push/send";
        const std::string body = data.value("body", "");
        const std::string title = data.value("title", "");

        int version = 11;

        for (const auto& token : device_tokens) {
            if (processed_tokens.find(token) != processed_tokens.end()) {
                continue;
            }

            processed_tokens.insert(token);

            if (token.rfind("ExponentPushToken", 0) == 0 || token.rfind("ExpoPushToken", 0) == 0) {
                json notification = {
                    {"to", token},
                    {"sound", "default"},
                    {"title", title},
                    {"body", body}
                };

                if (!data.is_null() && !data.empty()) {
                    notification["data"] = data;
                }

                payload.push_back(notification);
            }
        }

        if (payload.empty()) return;

        net::io_context ioc;
        ssl::context ctx{ssl::context::sslv23_client};

        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve(host, port);

        ssl::stream<tcp::socket> stream{ioc, ctx};

        net::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::post, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = payload.dump();
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        void(stream.shutdown(ec));
        if (ec == net::error::eof) ec ={};
        if (ec) throw beast::system_error{ec};
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

std::vector<std::string> get_server_recipient_tokens(const std::string& serverID, const std::string& deviceToken) {
    std::vector<std::string> tokens;

    try {
        Database db = connect_db();
        auto& conn = db.getConnection();
        pqxx::work txn(conn);
        std::string_view query =
            "SELECT t.user_id, t.token "
            "FROM device_tokens t "
            "JOIN user_servers us ON t.user_id = us.uid "
            "WHERE us.sid = $1 AND t.token != $2";
        pqxx::result r = txn.exec_params(query, serverID, deviceToken);

        for (const auto& row : r) {
            tokens.push_back(row["token"].as<std::string>());
        }
        
        txn.commit();

    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }

    return tokens;
}