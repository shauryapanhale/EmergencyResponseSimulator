cmake_minimum_required(VERSION 3.16)
project(ERS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB_RECURSE ERS_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_SOURCE_DIR}/src/models/*.cpp
    ${CMAKE_SOURCE_DIR}/src/services/*.cpp
)

add_library(ers_core STATIC ${ERS_CORE_SOURCES})
target_include_directories(ers_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(ers_console src/main_console.cpp)
target_link_libraries(ers_console PRIVATE ers_core)

enable_testing()
add_subdirectory(tests)