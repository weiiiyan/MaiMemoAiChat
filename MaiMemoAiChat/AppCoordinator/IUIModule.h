#pragma once

#include <QObject>
#include "../DataSync/MemEntry.h"

/**
 * @brief UI 模块抽象接口 — AppCoordinator 通过此接口驱动 UI 变更
 *
 * 双向协作：
 * - AppCoordinator 调用 IUIModule 方法更新 UI 状态
 * - IUIModule 通过信号触发 AppCoordinator 编排业务流程
 */
class IUIModule : public QObject
{
    Q_OBJECT

public:
    explicit IUIModule(QObject *parent = nullptr) : QObject(parent) {}
    ~IUIModule() override = default;

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

signals:
    /// 用户在对话中输入消息
    void messageSent(const QString &sessionId, const QString &content);

    /// 用户点击"开始复习"按钮
    void reviewRequested();

    /// 用户对复习卡片评分（目前通过对话形式，后续可能扩展独立 review 界面）
    void reviewAnswered(const QString &sessionId, qint64 cardId, int ease);

    /// 用户更改了 SRS 配置
    void settingsChanged(const SRSConfig &config);
};
