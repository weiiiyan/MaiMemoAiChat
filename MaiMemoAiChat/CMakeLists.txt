cmake_minimum_required(VERSION 3.19)
project(MaiMemoAiChat LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_subdirectory(Hold)
add_subdirectory(DataSync)

qt_add_executable(MaiMemoAiChat
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(MaiMemoAiChat
    PRIVATE
        Qt::Core
        Qt::Widgets
        Hold
        DataSync
)

install(TARGETS MaiMemoAiChat
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET MaiMemoAiChat
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
