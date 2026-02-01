#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>

class Logger {
public:
    static void init(const std::string& filename) {
        getInstance().logFile.open(filename, std::ios::app);
        if (!getInstance().logFile.is_open()) {
            std::cerr << "Could not open log file: " << filename << std::endl;
        }
    }

    static void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(getInstance().mtx);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << now_ms.count() << "] " << message;
        
        std::string out = ss.str();
        std::cout << out << std::endl;
        if (getInstance().logFile.is_open()) {
            getInstance().logFile << out << std::endl;
        }
    }

private:
    Logger() {}
    ~Logger() { if (logFile.is_open()) logFile.close(); }
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    std::ofstream logFile;
    std::mutex mtx;
};

#endif
