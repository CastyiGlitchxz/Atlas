#include <iostream>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;

inline int ping_server(std::string_view webhookKey) {
    try {
        const std::string_view host = "discord.com";
        const std::string_view port = "443";
        std::string_view target = webhookKey;

        int version = 11;

        std::string body = R"({"content": "Atlas Server is active."})";

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
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

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