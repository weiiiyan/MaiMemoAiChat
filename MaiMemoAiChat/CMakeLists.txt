cmake_minimum_required(VERSION 3.19)
project(MaiMemoAiChat LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_subdirectory(spdlog)
add_subdirectory(Hold)
add_subdirectory(DataSync)
add_subdirectory(SceneOrchestrator)
add_subdirectory(AppCoordinator)

qt_add_executable(MaiMemoAiChat
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    LogManage.h
)

# spdlog 宏（SPDLOG_DEBUG / SPDLOG_TRACE 等）的编译期级别
# 设为 DEBUG，确保调试日志不会被编译优化掉
target_compile_definitions(MaiMemoAiChat PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

target_link_libraries(MaiMemoAiChat
    PRIVATE
        Qt::Core
        Qt::Widgets
        spdlog
        Hold
        DataSync
        SceneOrchestrator
        AppCoordinator
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
