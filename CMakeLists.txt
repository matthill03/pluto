cmake_minimum_required(VERSION 3.20)

# Project name
set(APP_NAME pluto)
project(${APP_NAME})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(STRINGS ${CMAKE_HOME_DIRECTORY}/.env ENV_FILE)
foreach(VAR ${ENV_FILE})
    string(REGEX MATCH "^[^=]*" KEY ${VAR})
    string(REGEX REPLACE "^[^=]*=" "" VALUE ${VAR})
    set(${KEY} ${VALUE})
message("Setting ${KEY}, ${VALUE}")
endforeach()


find_library(VULKAN_LIB Vulkan PATHS ${VULKAN_SDK}/lib REQUIRED)
set(VULKAN_INCLUDE_DIR ${VULKAN_SDK}/include)
message(${VULKAN_INCLUDE_DIR})

# Source files
file(GLOB_RECURSE CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*/*.cpp")

# Vulkan settings - Mainly for MacOS
set(VK_ICD_FILENAMES ${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
set(LDFLAGS "-Wl,-rpath,${VULKAN_SDK}/lib/")

# Include FetchContent
include(FetchContent)

# Set the base directory for FetchContent
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/libs)

# Use FetchContent for GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Add executable
add_executable(${APP_NAME} ${CPP_FILES})

# Include directories
target_include_directories(${APP_NAME} PRIVATE
    ${VULKAN_INCLUDE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${APP_NAME} PRIVATE
    glfw
    ${VULKAN_LIB}
)

