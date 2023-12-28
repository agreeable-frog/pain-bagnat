cmake_minimum_required(VERSION 3.10)

project(pain-bagnat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

find_path(SHADERC_INCLUDE_DIRS NAMES shaderc/shaderc.hpp PATH_SUFFIXES shaderc)
find_library(SHADERC_LIBRARIES NAMES shaderc_combined)
if (SHADERC_INCLUDE_DIRS AND SHADERC_LIBRARIES)
    message("Found shaderc: ${SHADERC_LIBRARIES}")
else()
    message(FATAL_ERROR "shaderc not found")
endif()

configure_file(${PROJECT_SOURCE_DIR}/src/build_defs.hh.in ${CMAKE_CURRENT_BINARY_DIR}/build_defs.hh)

set(SOURCES ${PROJECT_SOURCE_DIR}/src/main.cc
            ${PROJECT_SOURCE_DIR}/src/shader_compiler.cc
            ${PROJECT_SOURCE_DIR}/src/instance.cc
            ${PROJECT_SOURCE_DIR}/src/display.cc
            ${PROJECT_SOURCE_DIR}/src/device.cc
            ${PROJECT_SOURCE_DIR}/src/swap_chain.cc
            ${PROJECT_SOURCE_DIR}/src/pipeline.cc)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${SHADERC_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw glm::glm ${SHADERC_LIBRARIES})