cmake_minimum_required(VERSION 3.10)

project(pain-bagnat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(SOURCES ${PROJECT_SOURCE_DIR}/src/main.cc
            ${PROJECT_SOURCE_DIR}/src/window.cc)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw glm::glm)