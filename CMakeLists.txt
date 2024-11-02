cmake_minimum_required(VERSION 3.8)
project(leven_engine)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE sources "${CMAKE_SOURCE_DIR}/src/*/*.c" 
			  "${CMAKE_SOURCE_DIR}/src/*/*.cpp")
add_executable(levent_engine "${sources}")
add_definitions(-DVULKAN_BACKEND)
target_include_directories(levent_engine PUBLIC "${CMAKE_SOURCE_DIR}/libs")
