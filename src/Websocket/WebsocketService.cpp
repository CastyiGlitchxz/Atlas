#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;

namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class WebsocketManager {
public:
    using EventCallback = std::function<void(const json&)>;

private:
    std::unordered_map<std::string, EventCallback> eventRegistry;

public:
    void on_event(const std::string& eventName, EventCallback callback) {
        eventRegistry[eventName] = callback;
    };

    void handle_incoming_message(const std::string& raw_frame) {
        try {
            
        } catch (const json::parse_error& e) {
            std::cerr << "Failed to parse network frame: " << e.what() << std::endl;
        }
    };
};

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

// int main() {
//     WebsocketManager ws;
//     ws.on_event("incoming", [](const json& data) {
//         data.value("Hello", "");
//     });
// }