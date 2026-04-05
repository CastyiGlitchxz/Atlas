#pragma once
#include <string>
#include <fstream>

struct cenvxx {
    struct PostInit {
        std::string directory;
        
        PostInit(const std::string& dir) : directory(dir) {}
        
        std::string trim(const std::string& str) {
            size_t start = str.find_first_not_of(" \t");
            size_t end   = str.find_last_not_of(" \t");
        
            if (start == std::string::npos) return "";
            return str.substr(start, end - start + 1);
        }
        
        bool line_contains_key(const std::string& line, const std::string& key_to_find) {
            size_t pos = line.find('|');
            if (pos == std::string::npos) return false;
        
            std::string key = trim(line.substr(0, pos));
            return key == key_to_find;
        }
        
        bool is_header(const std::string& line, const std::string& header) {
            std::string pattern = "% " + header + " %";
            return line.find(pattern) != std::string::npos;
        }
        
        std::string find_token(const std::string& header, const std::string& token) {
            std::ifstream env(directory);
            if (!env.is_open()) {
                return "Failed to open cenv file";
            }
        
            std::string line;
            bool inside_target_header = false;
        
            while (std::getline(env, line)) {
        
                // Enter header
                if (is_header(line, header)) {
                    inside_target_header = true;
                    continue;
                }
        
                // Stop if we enter a *different* header
                if (inside_target_header && line.find("%") != std::string::npos) {
                    // another header begins
                    break;
                }
        
                // Only search for key if we’re inside the header block
                if (inside_target_header && line_contains_key(line, token)) {
                    size_t pipe_pos = line.find('|');
                    std::string value = trim(line.substr(pipe_pos + 1));
                    return value;
                }
            }
        
            return "No key found";
        }
    };

    PostInit init(const std::string& directory) {
        return PostInit(directory);
    }
};