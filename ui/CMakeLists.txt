if(DSL_BUILD_LEGACY_WIN32_UI)
    add_executable(dsl_ui_legacy WIN32
        win32_main.cpp
        PipeClient.cpp
        PipeClient.h
        service_process.cpp
        service_process.h
        ui_formatting.cpp
        ui_formatting.h
    )

    target_include_directories(dsl_ui_legacy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(dsl_ui_legacy PRIVATE dsl_ipc)

    if(MSVC)
        target_compile_options(dsl_ui_legacy PRIVATE /W4 /permissive-)
    endif()
endif()

if(DSL_REGISTER_WINUI_SKELETON)
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
        winui3/main.cpp
        winui3/DualSenseLinkUI.vcxproj
        winui3/DualSenseLinkUI.vcxproj.filters
    )
endif()
