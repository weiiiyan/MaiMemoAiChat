# Main 组件：整个系统最外圈的组件，只做组装和移交，不含业务逻辑。
qt_add_executable(appMaiMemoAiChat
    main.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appMaiMemoAiChat PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMaiMemoAiChat
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 两个 mai_ui 系的库各有用处，缺一不可：
# - mai_ui      提供 UiModule 的 C++ 符号，Main 要构造它
# - mai_uiplugin 静态 QML 模块的注册与资源靠它保住。mai_ui 里的 QML 类型注册符号和
#   QML 资源没有任何 C++ 代码按符号引用，链接器会把它们当无用代码丢掉；Qt 为静态
#   QML 模块生成的 mai_uiplugin_init（内含 Q_IMPORT_PLUGIN 和资源 _init 对象）以
#   目标文件形式参与链接、不会被丢弃，而只有链接插件目标才会把它带上。
target_link_libraries(appMaiMemoAiChat
    PRIVATE
        Qt6::Quick
        mai_ui
        mai_uiplugin
)

install(TARGETS appMaiMemoAiChat
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
