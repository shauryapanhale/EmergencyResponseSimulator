cmake_minimum_required(VERSION 3.16)
project(ERS VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vendored SQLite amalgamation (defined before warning flags on purpose)
add_library(sqlite3_lib STATIC third_party/sqlite/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${CMAKE_SOURCE_DIR}/third_party/sqlite)
target_compile_definitions(sqlite3_lib PRIVATE SQLITE_OMIT_LOAD_EXTENSION=1)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB_RECURSE ERS_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_SOURCE_DIR}/src/models/*.cpp
    ${CMAKE_SOURCE_DIR}/src/services/*.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/*.cpp
)

add_library(ers_core STATIC ${ERS_CORE_SOURCES})
target_include_directories(ers_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ers_core PUBLIC sqlite3_lib)

add_executable(ers_console src/main_console.cpp)
target_link_libraries(ers_console PRIVATE ers_core)

add_executable(ers_persistence_demo src/main_persistence_demo.cpp)
target_link_libraries(ers_persistence_demo PRIVATE ers_core)

enable_testing()
add_subdirectory(tests)

# Optional Qt GUI (core library and tests never depend on Qt)
option(ERS_BUILD_GUI "Build the Qt GUI targets (needs Qt 6 Widgets)" ON)
if(ERS_BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        message(STATUS "Qt ${Qt6_VERSION} found: building GUI targets")
        add_subdirectory(src/ui)
    else()
        message(STATUS "Qt 6 not found: skipping GUI targets")
    endif()
endif()