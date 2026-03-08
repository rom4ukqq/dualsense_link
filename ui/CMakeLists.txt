add_executable(dsl_ui WIN32
    win32_main.cpp
    PipeClient.cpp
    PipeClient.h
)

target_include_directories(dsl_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(dsl_ui PRIVATE dsl_ipc)

if(MSVC)
    target_compile_options(dsl_ui PRIVATE /W4 /permissive-)
endif()

add_custom_target(dsl_ui_winui_skeleton SOURCES
    App.xaml
    App.xaml.h
    App.xaml.cpp
    MainWindow.xaml
    MainWindow.xaml.h
    MainWindow.xaml.cpp
    ControllerDashboard.xaml
    ControllerDashboard.xaml.h
    ControllerDashboard.xaml.cpp
)
