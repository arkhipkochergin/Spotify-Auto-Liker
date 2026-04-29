cmake_minimum_required(VERSION 3.16)

project(SpotifyLikerUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(SpotifyLikerUI WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(SpotifyLikerUI PRIVATE Qt6::Widgets Qt6::Network)
