cmake_minimum_required(VERSION 3.14)
project(PigeonMail CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(pigeon_mail
    src/main.cpp
    src/Pigeon.cpp
    src/Message.cpp
    src/Player.cpp
    src/PigeonMailApp.cpp
)

target_compile_options(pigeon_mail PRIVATE -Wall -Wextra -O2)
