#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

using json = nlohmann::json;

class HTTPManager {
public:
    using HTTPRoute = std::function<http::response<http::string_body>(const http::request<http::string_body>&)>;
    std::map<std::string, HTTPRoute> routes;
    
public:
    void add_route(const std::string& route, const HTTPRoute& callback) {
        routes[route] = callback;
    };
};