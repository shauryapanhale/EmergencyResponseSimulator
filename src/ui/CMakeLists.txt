set(CMAKE_AUTOMOC ON)

add_executable(ers_hello_qt hello_qt.cpp)
target_link_libraries(ers_hello_qt PRIVATE ers_core Qt6::Widgets)