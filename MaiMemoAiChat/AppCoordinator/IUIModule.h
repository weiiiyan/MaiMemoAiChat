#pragma once

#include <QString>

/**
 * @brief UI 模块纯虚接口 — AppCoordinator 通过此接口驱动 UI 变更
 *
 * 双向协作：
 * - AppCoordinator 调用 IUIModule 方法更新 UI 状态
 * - 实现类（MainWindow）通过自身信号触发 AppCoordinator 编排业务流程
 *
 * 注意：此接口不继承 QObject，信号由实现类（MainWindow）直接声明，
 * 以避免 QMainWindow + QObject 的多继承菱形问题。
 */
class IUIModule
{
public:
    virtual ~IUIModule() = default;

    /// 展示指定会话的对话界面（复习/学习）
    virtual void showChatView(const QString &sessionId) = 0;

    /// 在对话中追加一条消息（用户消息或 AI 消息/流式片段）
    virtual void showMessage(const QString &sessionId, const QString &message) = 0;

    /// 对话结束后展示总结界面
    virtual void showSummary(const QString &sessionId) = 0;

    /// 更新到期条目数（角标/提示）
    virtual void setDueEntryCount(int count) = 0;

    /// 更新初始化状态指示
    virtual void setInitialized(bool initialized) = 0;
};
