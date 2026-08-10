#include <fstream>
#include <iostream>

inline std::string handle_images(std::vector<unsigned char> buffer, std::pair<size_t, size_t> result, const std::string& image_name) {
    std::string assets_folders = "../assets/users/photos/";
    std::string fpath = assets_folders + image_name;
    std::string pretty_path;

    std::ofstream image(fpath, std::ios::binary);
    image.write(reinterpret_cast<char*>(buffer.data()), result.first);
    image.close();

    std::cout << "[Server] Profile image saved!\n";

    size_t pos = fpath.find("..");
    if (pos != std::string::npos)
        pretty_path = fpath.substr(pos + 2);

    return pretty_path;
}