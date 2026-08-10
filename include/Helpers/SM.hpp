#pragma once

#include <string>
#include "../cenv.hpp"

inline cenvxx clangxx;
inline auto cenv = clangxx.init("../config/cenv");

inline std::string secret = cenv.find_token("secrets", "securekey");