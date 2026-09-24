cmake_minimum_required(VERSION 3.16)

project(MaiMemoAiChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.10)

# QML 模块的构建产物按其 URI 组织（如 qml/MaiMemoAiChat/Ui/），不再跟随源码目录。
# 这样源码目录可以自由布局，同时 qmllint/qmlcachegen 会自动把该目录加入 import path。
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

add_subdirectory(domain)

# ui 必须先于 Main：Main 的可执行目标要链接 mai_ui 生成出的插件目标 mai_uiplugin
add_subdirectory(framework/ui)
add_subdirectory(framework/Main)

enable_testing()
add_subdirectory(tests)
