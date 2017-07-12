# Author: Luxoft Professional
# Date: 09 February 2017

cmake_minimum_required(VERSION 3.0)
macro(use_cxx11)
            if (CMAKE_VERSION VERSION_LESS "3.1")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" CACHE STRING "compile flags" FORCE)
            else()
                        set (CMAKE_CXX_STANDARD 11)
                        set (CMAKE_CXX_STANDARD_REQUIRED ON)
            endif()
endmacro(use_cxx11)
 
use_cxx11()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g")

# Project name
project (CliCentralApp)

# Define AUDIO_MANAGER_CLIENT_TEST directory
if (NOT CLI_CENTRAL_APP_DIR)
    set(CLI_CENTRAL_APP_DIR "${PROJECT_SOURCE_DIR}")
endif (NOT CLI_CENTRAL_APP_DIR)


 # Set Common-Api dir
set(COMMON_API_DIR "${REPOSITORY_DIR}/common-api")
set(COMMON_API_BIN_DIR "${COMMON_API_DIR}/bin")


include_directories(
		${CLI_CENTRAL_APP_DIR}/inc
		${CLI_CENTRAL_APP_DIR}/src
)

file(GLOB_RECURSE SOURCE_FILES_GENERATED ${CLI_CENTRAL_APP_DIR}/src/*.cpp)


# Define output directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CLI_CENTRAL_APP_DIR}/bin)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CLI_CENTRAL_APP_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CLI_CENTRAL_APP_DIR}/bin)


# Make .so
add_executable(CliAutoTestApp
                ${SOURCE_FILES_GENERATED}
                main.cpp
)


# Link with libraries
target_link_libraries (CliAutoTestApp pthread dl)
