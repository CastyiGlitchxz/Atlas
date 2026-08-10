#include <iostream>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;
using json = nlohmann::json;

inline int discord_send_message(const std::string& webhookKey, const std::string& username, const std::string& message, const std::string& avatarURL) {
    try {
        const std::string host = "discord.com";
        const std::string port = "443";
        std::string target = webhookKey;

        int version = 11;

        json body = {
            {"content", message},
            {"username", username},
            {"avatar_url", "https://lawrenz-laptop.tail7bc346.ts.net" + avatarURL}
        };

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
        req.body() = body.dump();
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