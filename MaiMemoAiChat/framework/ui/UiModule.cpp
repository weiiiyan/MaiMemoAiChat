#include "UiModule.h"

namespace {
// 本模块自身的 QML 标识，只有 UI 模块需要知道
constexpr auto UI_URI = "MaiMemoAiChat.Ui";
constexpr auto ROOT_TYPE = "Main";
}

bool UiModule::start()
{
    m_engine.loadFromModule(UI_URI, ROOT_TYPE);

    // loadFromModule 无返回值，根组件是否建成只能从引擎里查
    return !m_engine.rootObjects().isEmpty();
}
