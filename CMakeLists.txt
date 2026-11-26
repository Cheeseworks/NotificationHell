cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(NotificationHell VERSION 1.1.2)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/cubicstudios.horriblemenu)
target_include_directories(${PROJECT_NAME} PRIVATE src include)

if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define the GEODE_SDK environment variable to point to Geode.")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

option(CW_TESTING "Enable local testing mode" ON)
if(CW_TESTING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CW_TESTING=1)
endif()

CPMAddPackage("gh:dankmeme01/cue#main")
CPMAddPackage("gh:Cheeseworks/rhatts-geode-sillies#main")
target_link_libraries(${PROJECT_NAME} cue rhatt)
