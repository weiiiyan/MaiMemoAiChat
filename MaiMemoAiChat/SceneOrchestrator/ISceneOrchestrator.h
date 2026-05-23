#pragma once

#include <QObject>
#include "SceneTypes.h"

/**
 * @brief 学习场景编排模块主接口
 *
 * 位于 AppCoordinator 与 AI 服务之间，核心职责:
 * - 创建交互式学习对话会话（SceneSession），为 AI 设定角色 system prompt
 * - 管理多轮交互：收发消息、流式响应、对话生命周期
 * - 抽象 AI 调用细节，对内保持接口稳定
 *
 * 所有 AI 通信均为异步，结果通过信号返回。
 */
class ISceneOrchestrator : public QObject
{
    Q_OBJECT

public:
    explicit ISceneOrchestrator(QObject *parent = nullptr) : QObject(parent) {}
    ~ISceneOrchestrator() override = default;

    /// 配置 AI 服务参数（首次使用前调用）
    virtual void configure(const SceneConfig &config) = 0;

    /// 创建学习对话会话。返回 sessionId（同步），AI 就绪后 emit sessionCreated
    virtual QString createSession(const QList<MemEntry> &entries,
                                  SceneType sceneType,
                                  const SceneConfig &config) = 0;

    /// 在指定会话中发送用户消息。AI 回复通过 responseStreaming/responseComplete 信号异步返回
    virtual void sendMessage(const QString &sessionId, const QString &content) = 0;

    /// 停止当前 AI 响应生成（用户中途打断）
    virtual void stopResponse(const QString &sessionId) = 0;

    /// 结束学习对话，释放相关资源。emit sessionEnded
    virtual void endSession(const QString &sessionId) = 0;

    /// 获取会话信息，用于恢复会话或查询状态。会话不存在返回空 SceneSession (id 为空)
    virtual SceneSession getSession(const QString &sessionId) const = 0;

signals:
    /// 学习对话创建完成，AI 已就绪并返回开场白。content 为 AI 的开场白消息
    void sessionCreated(const QString &sessionId, const QString &openingMessage);

    /// AI 回复的流式片段，UI 逐字展示
    void responseStreaming(const QString &sessionId, const QString &chunk);

    /// AI 回复完成，携带完整消息内容
    void responseComplete(const QString &sessionId, const QString &message);

    /// 会话过程中出错
    void sessionError(const QString &sessionId, const QString &error);

    /// 对话已结束
    void sessionEnded(const QString &sessionId);
};
