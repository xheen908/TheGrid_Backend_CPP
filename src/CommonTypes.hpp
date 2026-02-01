#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

#include <string>

struct PerSocketData {
    std::string username;
    std::string charName;
    std::string mapName;
    bool isAuthenticated = false;
    bool isGM = false;
};

#endif
