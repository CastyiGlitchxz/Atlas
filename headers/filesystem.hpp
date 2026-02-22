#pragma once
#include <boost/beast/core/flat_buffer.hpp>

std::string handle_images(std::vector<unsigned char> buffer, std::pair<size_t, size_t> result, const std::string& image_name);