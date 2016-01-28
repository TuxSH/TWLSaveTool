cmake_minimum_required(VERSION 2.8)
project(TWLSaveTool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -ffast-math -std=gnu++11") # Use the devkitArm suggested flags. This is not mandatory.

# Note that you must copy the cmake folder and the DevkitArm3DS.cmake file in this directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake) # Add the cmake folder to the modules paths, so that we can use the tools and find_package for ctrulib
include(Tools3DS) # Include all the macros and tools needed for 3ds development.

find_package(CTRULIB REQUIRED) # Look for ctrulib

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCE_FILES
	source/*
)

file(GLOB_RECURSE HEADER_FILES
	include/*
)

add_executable(TWLSaveTool ${SOURCE_FILES} ${HEADER_FILES}) # Create the elf file
target_link_libraries(TWLSaveTool 3ds::ctrulib) # Link ctrulib, this will also setup the include directories !

if(CIA)
	add_cia_target(TWLSaveTool "${PROJECT_SOURCE_DIR}/app/build-cia.rsf" "${PROJECT_SOURCE_DIR}/app/banner.cfgx" 
	"${PROJECT_SOURCE_DIR}/app/DSLite_BootSound.cwav" "TWLSaveTool" "TWLSaveTool" "TuxSH" "${PROJECT_SOURCE_DIR}/app/IconLarge.png")
else()
	add_3dsx_target(TWLSaveTool "TWLSaveTool" "A tool to backup and restore save files from NDS cartridges (PokéTransporter required)" "TuxSH" "${PROJECT_SOURCE_DIR}/app/IconLarge.png")
endif()