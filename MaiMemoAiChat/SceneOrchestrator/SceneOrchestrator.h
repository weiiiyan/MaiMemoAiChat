#pragma once

#include "ISceneOrchestrator.h"
#include <QHash>
#include <memory>

class IAIManager;

/**
 * @brief ISceneOrchestrator 具体实现
 *
 * 内部架构:
 *   ISceneOrchestrator ──> IAIManager ──> WenXinAiManager / QianWenAiManager
 *
 * 每个 SceneSession 持有一个 IAIManager 实例，独立管理对话状态和对话历史。
 * AI 回复通过信号转接到 AppCoordinator，携带 sessionId 标识。
 */
class SceneOrchestrator : public ISceneOrchestrator
{
    Q_OBJECT

public:
    explicit SceneOrchestrator(QObject *parent = nullptr);
    ~SceneOrchestrator() override = default;

    void configure(const SceneConfig &config) override;

    QString createSession(const QList<MemEntry> &entries,
                          SceneType sceneType,
                          const SceneConfig &config) override;

    void sendMessage(const QString &sessionId, const QString &content) override;
    void stopResponse(const QString &sessionId) override;
    void endSession(const QString &sessionId) override;
    SceneSession getSession(const QString &sessionId) const override;

private slots:
    void onAiResponseStreaming(const QString &chunk);
    void onAiResponseComplete(const QString &fullMessage);
    void onAiResponseError(const QString &error);

private:
    /// 创建 IAIManager 实例（工厂方法，根据 config.aiProvider 选择实现）
    static IAIManager *createAiManager(const SceneConfig &config, QObject *parent);

    /// 根据 SceneType + entries 构建 system prompt
    static QString buildSystemPrompt(SceneType type, const QList<MemEntry> &entries);

    /// 生成唯一 sessionId
    static QString generateSessionId();

    /// 解析信号发送者对应的 sessionId
    QString sessionIdForSender() const;

    /// 当前会话 (SceneOrchestrator 单会话模型——同一时间只有一个活跃会话)
    QString m_currentSessionId;
    QHash<QString, SceneSession> m_sessions;          ///< sessionId → SceneSession
    QHash<QString, IAIManager *> m_aiManagers;        ///< sessionId → IAIManager
    QHash<IAIManager *, QString> m_managerToSession;  ///< IAIManager* → sessionId (反向索引)
};
