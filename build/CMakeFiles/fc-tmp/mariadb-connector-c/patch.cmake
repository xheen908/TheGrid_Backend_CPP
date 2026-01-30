cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

message(VERBOSE "Executing patch step for mariadb-connector-c")

block(SCOPE_FOR VARIABLES)

execute_process(
  WORKING_DIRECTORY "C:/Users/xheen908/Documents/thegrid_backend_c/build/_deps/mariadb-connector-c-src"
  COMMAND_ERROR_IS_FATAL LAST
  COMMAND  [====[powershell]====] [====[-Command]====] [====[Get-ChildItem -Recurse CMakeLists.txt | ForEach-Object { (Get-Content $_.FullName) -replace 'VERSION 2.8', 'VERSION 3.5' -replace 'VERSION 2.4', 'VERSION 3.5' | Set-Content $_.FullName }]====]
)

endblock()
