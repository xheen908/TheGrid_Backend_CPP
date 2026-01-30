#ifndef SERVER_HPP
#define SERVER_HPP

#include "App.h"
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>
#include "CommonTypes.hpp"

using json = nlohmann::json;

class AuthServer {
public:
    AuthServer();
    void start(int port);
    void stop();
private:
    bool running;
};

class WorldServer {
public:
    WorldServer();
    void start(int port);
    void stop();
    void tick();
    static void defer(std::function<void()> cb);
private:
    bool running;
};

#endif
