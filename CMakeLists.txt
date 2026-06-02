cmake_minimum_required(VERSION 3.14)

project(PigeonMail LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(curl STATIC IMPORTED)

set_target_properties(curl PROPERTIES
    IMPORTED_LOCATION             "${CMAKE_SOURCE_DIR}/external/curl/lib/libcurl.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/curl/include"
)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE external/nlohmann)

add_executable(
    pigeon_mail
    src/main.cpp
    src/Pigeon.cpp
    src/Message.cpp
    src/Player.cpp
    src/ConsoleUI.cpp
    src/PrettyUI.cpp
    src/App.cpp
)

target_include_directories(pigeon_mail
    PRIVATE include
)

target_link_libraries(pigeon_mail
    PRIVATE
        curl
        nlohmann_json
)