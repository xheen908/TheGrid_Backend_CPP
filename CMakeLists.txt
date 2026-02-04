cmake_minimum_required(VERSION 3.10...3.30)
project(TheGridBackendC)

set(CMAKE_CXX_STANDARD 20)

# Set install paths to avoid MariaDB install errors
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
set(CMAKE_SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)

include(FetchContent)

# Fetch nlohmann_json
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Fetch MariaDB Connector/C
FetchContent_Declare(
  mariadb-connector-c
  GIT_REPOSITORY https://github.com/mariadb-corporation/mariadb-connector-c.git
  GIT_TAG        v3.3.5
  PATCH_COMMAND powershell -Command "Get-ChildItem -Recurse CMakeLists.txt | ForEach-Object { (Get-Content $_.FullName) -replace 'VERSION 2.8', 'VERSION 3.5' -replace 'VERSION 2.4', 'VERSION 3.5' | Set-Content $_.FullName }"
)

# FORCE STATIC BUILD FOR MARIADB
set(WITH_CURL OFF CACHE BOOL "" FORCE)
set(WITH_DYNCOL OFF CACHE BOOL "" FORCE)
set(WITH_MYSQLCOMPAT ON CACHE BOOL "" FORCE)
set(WITH_SSL OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(WITH_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(MARIADB_STATIC ON CACHE BOOL "" FORCE)
set(MARIADB_LINK_DYNAMIC OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBDIR "lib" CACHE STRING "")
set(INSTALL_BINDIR "bin" CACHE STRING "")
set(INSTALL_INCLUDEDIR "include" CACHE STRING "")

FetchContent_MakeAvailable(mariadb-connector-c)

# Fetch uSockets
FetchContent_Declare(
  usockets
  GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
  GIT_TAG        v0.8.8
)
FetchContent_MakeAvailable(usockets)

# Fetch uWebSockets
FetchContent_Declare(
  uwebsockets
  GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
  GIT_TAG        v20.48.0
)
FetchContent_MakeAvailable(uwebsockets)

# Executables
add_executable(thegrid_auth src/AuthMain.cpp src/Database.cpp src/logic/GameLogicScaling.cpp)
add_executable(thegrid_world 
    src/WorldMain.cpp 
    src/Server.cpp 
    src/GameState.cpp 
    src/Database.cpp
    src/SocketHandlers.cpp
    src/GMCommands.cpp
    src/ChatHandlers.cpp
    src/gm/GMUtil.cpp
    src/gm/GMPlayerCommands.cpp
    src/gm/GMTeleportCommands.cpp
    src/gm/GMSystemCommands.cpp
    src/logic/GameLogicScaling.cpp
    src/logic/GameLogicCombat.cpp
    src/logic/GameLogicPlayer.cpp
    src/logic/MobAI.cpp
)

# Fetch libuv
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  libuv
  GIT_REPOSITORY https://github.com/libuv/libuv.git
  GIT_TAG        v1.48.0
)
FetchContent_MakeAvailable(libuv)

# SHARED SETUP
set(SHARED_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${uwebsockets_SOURCE_DIR}/src"
    "${usockets_SOURCE_DIR}/src"
    "${libuv_SOURCE_DIR}/include"
    "${mariadb-connector-c_SOURCE_DIR}/include"
    "${mariadb-connector-c_BINARY_DIR}/include"
)

file(GLOB USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/*.c")
set(USOCKETS_UV_SRC "${usockets_SOURCE_DIR}/src/eventing/libuv.c")

target_include_directories(thegrid_auth PRIVATE ${SHARED_INCLUDES})
target_include_directories(thegrid_world PRIVATE ${SHARED_INCLUDES})

target_sources(thegrid_auth PRIVATE ${USOCKETS_SOURCES} "${USOCKETS_UV_SRC}")
target_sources(thegrid_world PRIVATE ${USOCKETS_SOURCES} "${USOCKETS_UV_SRC}")

# USE THE STATIC TARGET 'mariadbclient' instead of 'libmariadb'
target_link_libraries(thegrid_auth PRIVATE nlohmann_json::nlohmann_json uv_a mariadbclient)
target_link_libraries(thegrid_world PRIVATE nlohmann_json::nlohmann_json uv_a mariadbclient)

# Common Definitions
set(COMMON_DEFS
    UWS_WITHOUT_SSL
    UWS_NO_ZLIB
    LIBUS_NO_SSL
    LIBUS_USE_LIBUV
)

if(WIN32)
    set(SYS_LIBS ws2_32 iphlpapi user32 shell32 advapi32 crypt32)
    list(APPEND COMMON_DEFS _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_link_libraries(thegrid_auth PRIVATE ${SYS_LIBS})
    target_link_libraries(thegrid_world PRIVATE ${SYS_LIBS})
endif()

target_compile_definitions(thegrid_auth PRIVATE ${COMMON_DEFS})
target_compile_definitions(thegrid_world PRIVATE ${COMMON_DEFS})
