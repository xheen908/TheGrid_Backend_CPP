# Install script for directory: C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/xheen908/Documents/thegrid_backend_c/build/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb_com.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mysql.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb_stmt.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/ma_pvio.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/ma_tls.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-build/include/mariadb_version.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/ma_list.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/errmsg.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb_dyncol.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb_ctype.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb_rpl.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mysql" TYPE FILE FILES
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mysql/client_plugin.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mysql/plugin_auth_common.h"
    "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mysql/plugin_auth.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mariadb" TYPE FILE FILES "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src/include/mariadb/ma_io.h")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-build/include/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
