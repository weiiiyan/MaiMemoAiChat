#pragma once

#include "IAppCoordinator.h"
#include "IUIModule.h"

class IDataSync;
class ISceneOrchestrator;
class Hold;

/**
 * @brief IAppCoordinator 具体实现
 *
 * 内部持有所有子模块引用：
 *   IAppCoordinator ──> ISceneOrchestrator
 *                  │──> IDataSync
 *                  └──> Hold
 *
 * 与 IUIModule 双向协作：
 * - 监听 IUIModule 信号（messageSent / reviewRequested / reviewAnswered / settingsChanged）
 * - 调用 IUIModule 方法（showChatView / showMessage / showSummary）驱动 UI
 * - 连接 ISceneOrchestrator → 转发 AI 响应到 UI
 */
class AppCoordinator : public IAppCoordinator
{
    Q_OBJECT

public:
    explicit AppCoordinator(QObject *parent = nullptr);
    ~AppCoordinator() override;

    bool initialize(QObject *uiObject, IUIModule *ui, const AppConfig &config) override;
    void shutdown() override;
    bool isInitialized() const override;
    void startReview() override;
    int dueEntryCount() const override;

private slots:
    void onMessageSent(const QString &sessionId, const QString &content);
    void onReviewRequested();
    void onReviewAnswered(const QString &sessionId, qint64 cardId, int ease);
    void onSettingsChanged(const SRSConfig &config);

    void onDataSyncConnected();
    void onDataSyncDisconnected();
    void onDataSyncError(const SyncError &error);

    void onSessionCreated(const QString &sessionId, const QString &openingMessage);
    void onResponseStreaming(const QString &sessionId, const QString &chunk);
    void onResponseComplete(const QString &sessionId, const QString &message);
    void onSessionError(const QString &sessionId, const QString &error);
    void onSessionEnded(const QString &sessionId);

private:
    void saveLastSession(const QString &sessionId);
    QString loadLastSession() const;
    void saveConfig() const;
    void loadConfig(AppConfig &config) const;
    void createSubModules(const AppConfig &config);
    void connectUiSignals();
    void connectSceneOrchestratorSignals();
    void restoreLastSession();

    bool m_initialized = false;
    QString m_currentSessionId;               ///< 当前活跃的学习会话 ID

    QObject *m_uiObject = nullptr;             ///< 用于 connect() 信号连接（MainWindow）
    IUIModule *m_ui = nullptr;                 ///< 用于调用 UI 方法
    Hold *m_hold = nullptr;
    IDataSync *m_dataSync = nullptr;
    ISceneOrchestrator *m_sceneOrchestrator = nullptr;

    AppConfig m_config;
};
