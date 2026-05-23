#pragma once

#include <QObject>
#include <QList>
#include "SceneTypes.h"

/**
 * @brief AI 服务抽象接口 — 屏蔽文心一言/通义千问的差异
 *
 * 每个实现类封装一种 AI 服务商的 HTTP API 调用细节（认证、流式解析）。
 * SceneOrchestrator 通过此接口与 AI 交互，不直接依赖具体服务商。
 */
class IAIManager : public QObject
{
    Q_OBJECT

public:
    explicit IAIManager(QObject *parent = nullptr) : QObject(parent) {}
    ~IAIManager() override = default;

    /// 配置 AI 服务参数（认证凭据、模型等）
    virtual void configure(const SceneConfig &config) = 0;

    /// 发送消息列表（含 system prompt 和历史），AI 回复通过信号异步返回
    virtual void sendMessage(const QList<ChatMessage> &messages) = 0;

    /// 停止当前正在生成的回复
    virtual void stopResponse() = 0;

    /// 是否正在生成回复
    virtual bool isBusy() const = 0;

signals:
    /// AI 回复的流式片段，delta 内容
    void responseStreaming(const QString &chunk);

    /// AI 回复完整结果
    void responseComplete(const QString &fullMessage);

    /// AI 调用出错
    void responseError(const QString &error);
};
