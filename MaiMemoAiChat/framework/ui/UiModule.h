#pragma once

#include <QQmlApplicationEngine>

/**
 * @brief UI 模块的 C++ 入口，封装 QML 界面的启动。
 *
 * QML 引擎的创建、根组件的加载方式、界面本体是什么类型，
 * 都是 UI 模块自己的实现细节，不应泄漏到上层。
 */
class UiModule
{
public:
    /**
     * @brief 加载并显示界面。
     * @return 根组件创建成功返回 true；失败返回 false，如何处理由调用方决定
     */
    bool start();

private:
    // 引擎必须与界面同寿命：QML 对象树归它所有
    QQmlApplicationEngine m_engine;
};
