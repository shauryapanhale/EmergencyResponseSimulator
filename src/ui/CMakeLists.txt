set(CMAKE_AUTOMOC ON)

add_library(ers_qt STATIC
    ApplicationFacade.cpp
    MainWindow.cpp
    MapView.cpp
    ${CMAKE_SOURCE_DIR}/include/ers/ui/ApplicationFacade.h
    ${CMAKE_SOURCE_DIR}/include/ers/ui/MainWindow.h
    ${CMAKE_SOURCE_DIR}/include/ers/ui/MapView.h
)
target_include_directories(ers_qt PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ers_qt PUBLIC ers_core Qt6::Core Qt6::Widgets)

add_executable(ers_hello_qt hello_qt.cpp)
target_link_libraries(ers_hello_qt PRIVATE ers_core Qt6::Widgets)

add_executable(ers_gui main_gui.cpp)
target_link_libraries(ers_gui PRIVATE ers_qt Qt6::Widgets)

add_executable(ApplicationFacadeTest tests/ApplicationFacadeTest.cpp)
target_link_libraries(ApplicationFacadeTest PRIVATE ers_qt Qt6::Test)
add_test(NAME ApplicationFacadeTest COMMAND ApplicationFacadeTest)

# Let ctest find the Qt DLLs without editing PATH by hand
get_filename_component(ERS_QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
    set_tests_properties(ApplicationFacadeTest PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${ERS_QT_BIN_DIR}")
endif()