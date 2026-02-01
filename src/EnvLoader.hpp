#ifndef ENV_LOADER_HPP
#define ENV_LOADER_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

class EnvLoader {
public:
    static void load(const std::string& filename = ".env") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Basic trimming (remove quotes/spaces if any)
                trim(key);
                trim(value);

                if (!key.empty()) {
                    std::cout << "[ENV] Setting " << key << " = " << (key.find("PASS") != std::string::npos ? "****" : value) << std::endl;
#ifdef _WIN32
                    std::string envStr = key + "=" + value;
                    _putenv(envStr.c_str());
#else
                    setenv(key.c_str(), value.c_str(), 1);
#endif
                }
            }
        }
    }

private:
    static void trim(std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\r\n\"'"));
        s.erase(s.find_last_not_of(" \t\r\n\"'") + 1);
    }
};

#endif
