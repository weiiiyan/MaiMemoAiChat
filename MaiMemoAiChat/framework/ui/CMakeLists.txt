# UI 模块：QML 界面，编成静态库供 Main 组件链接。
# 按整洁架构，UI 属于最外层的细节，依赖 Qt 而不被任何内层模块依赖。
qt_add_library(mai_ui STATIC)

qt_add_qml_module(mai_ui
    URI MaiMemoAiChat.Ui
    QML_FILES
        Main.qml
    SOURCES
        UiModule.h
        UiModule.cpp
)

# include 目录设为 framework/，使用方写 #include "ui/UiModule.h"，
# 引用处一眼能看出符号来自哪个模块
target_include_directories(mai_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(mai_ui
    PUBLIC
        Qt6::Quick
)
