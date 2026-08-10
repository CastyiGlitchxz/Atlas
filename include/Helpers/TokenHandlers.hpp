#include <boost/beast/http.hpp>
#include "SM.hpp"
#include <string>
#include <jwt-cpp/jwt.h>

namespace beast = boost::beast;
namespace http = beast::http;

inline std::string decode_token(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret});
        verifier.verify(decoded);

        return decoded.get_subject();
    } catch (const std::exception& e) {
        std::cerr << "[JWT Error] Failed to decode/verify token: " << e.what() << std::endl;
        return "";
    }
}

inline std::string get_user_id_from_cookie(const http::request<http::string_body>& req) {
    if (!req.count(http::field::cookie)) {
        throw std::runtime_error("Authorization cookie missing.");
    }

    std::string cookie_header = req[http::field::cookie];
    std::string token;
    size_t start = cookie_header.find("token=");
    if (start != std::string::npos) {
        start += 6;
        size_t end = cookie_header.find(";", start);
        if (end == std::string::npos) {
            token = cookie_header.substr(start);
        } else {
            token = cookie_header.substr(start, end - start);
        }
    } else {
        throw std::runtime_error("Auth token not found in cookie header.");
    }

    return decode_token(token);
}

inline std::string parse_bearer_token(const http::request<http::string_body>& req) {
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