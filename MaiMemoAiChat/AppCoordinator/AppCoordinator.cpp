#include "AppCoordinator.h"
#include "../Hold/Hold.h"
#include "../DataSync/IDataSync.h"
#include "../DataSync/DataSync.h"
#include "../DataSync/AnkiConnectEngine.h"
#include "../SceneOrchestrator/ISceneOrchestrator.h"
#include "../SceneOrchestrator/SceneOrchestrator.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <spdlog/spdlog.h>

namespace {
spdlog::logger* logger() {
    static auto instance = spdlog::get("AppCoordinator");
    return instance.get();
}
} // namespace

// ── Hold 存储 key ──

static const QStringList kAppConfigId   = {QStringLiteral("app")};
static const QString     kConfigName    = QStringLiteral("config");
static const QStringList kStateId       = {QStringLiteral("app")};
static const QString     kLastSession   = QStringLiteral("last_session");

// ── 构造 / 析构 ──

AppCoordinator::AppCoordinator(QObject *parent)
    : IAppCoordinator(parent)
{
}

AppCoordinator::~AppCoordinator()
{
    shutdown();
}

// ── 初始化 / 关闭 ──

bool AppCoordinator::initialize(QObject *uiObject, IUIModule *ui, const AppConfig &config)
{
    if (m_initialized)
        return false;

    m_uiObject = uiObject;
    m_ui = ui;
    m_config = config;

    SPDLOG_LOGGER_INFO(logger(), "initializing: dataDir={}, provider={}, model={}, deck={}",
                config.dataDir.toStdString(),
                config.sceneConfig.aiProvider == AiProvider::QianWen ? "QianWen" : "WenXin",
                config.sceneConfig.model.toStdString(),
                config.srsConfig.deckName.toStdString());

    loadConfig(m_config);
    createSubModules(m_config);
    connectUiSignals();
    connectSceneOrchestratorSignals();

    m_initialized = true;

    if (m_ui) {
        m_ui->showStatus(QStringLiteral("Connecting to Anki..."));
        m_ui->setDueEntryCount(dueEntryCount());
        m_ui->appendSystemMessage(
            QStringLiteral("App initialized.\n"
                           "AI provider: %1, model: %2\n"
                           "SRS: %3 @ %4")
                .arg(config.sceneConfig.aiProvider == AiProvider::QianWen
                         ? QStringLiteral("QianWen") : QStringLiteral("WenXin"))
                .arg(config.sceneConfig.model)
                .arg(config.srsConfig.deckName)
                .arg(config.srsConfig.url));
    }

    // 异步连接 Anki——connected 信号到来时才 fullSync
    if (m_dataSync)
        m_dataSync->connectToEngine(m_config.srsConfig);

    restoreLastSession();

    return true;
}

void AppCoordinator::shutdown()
{
    if (!m_initialized)
        return;

    SPDLOG_LOGGER_INFO(logger(), "shutting down");

    // 结束当前活跃会话
    if (!m_currentSessionId.isEmpty() && m_sceneOrchestrator) {
        m_sceneOrchestrator->endSession(m_currentSessionId);
    }

    // 保存状态
    saveLastSession(m_currentSessionId);
    saveConfig();

    // 断开 SRS 引擎
    if (m_dataSync)
        m_dataSync->disconnectFromEngine();

    m_currentSessionId.clear();
    m_initialized = false;

    if (m_ui) {
        m_ui->showStatus(QStringLiteral("Offline"));
        m_ui->setDueEntryCount(0);
    }
}

bool AppCoordinator::isInitialized() const
{
    return m_initialized;
}

// ── 复习流程 ──

void AppCoordinator::startReview()
{
    if (!m_initialized || !m_dataSync || !m_sceneOrchestrator)
        return;

    if (!m_currentSessionId.isEmpty()) {
        m_sceneOrchestrator->endSession(m_currentSessionId);
        m_currentSessionId.clear();
    }

    QList<MemEntry> dueEntries = m_dataSync->getDueEntries(m_config.dueEntryLimit);
    if (dueEntries.isEmpty()) {
        SPDLOG_LOGGER_INFO(logger(), "startReview: no due entries");
        if (m_ui) {
            m_ui->appendSystemMessage(
                QStringLiteral("No due entries. Please sync with Anki first (Anki must be running with AnkiConnect plugin)."));
        }
        return;
    }

    SPDLOG_LOGGER_INFO(logger(), "startReview: {} due entries, sceneType={}", dueEntries.size(), static_cast<int>(m_config.defaultSceneType));
    QString sessionId = m_sceneOrchestrator->createSession(
        dueEntries, m_config.defaultSceneType, m_config.sceneConfig);

    m_currentSessionId = sessionId;
    saveLastSession(sessionId);

    if (m_ui)
        m_ui->showStatus(QStringLiteral("Creating session..."));
}

int AppCoordinator::dueEntryCount() const
{
    if (!m_dataSync)
        return 0;
    return m_dataSync->getDueEntries(m_config.dueEntryLimit).size();
}

// ── IUIModule 信号处理 ──

void AppCoordinator::onMessageSent(const QString &sessionId, const QString &content)
{
    if (!m_sceneOrchestrator)
        return;
    m_sceneOrchestrator->sendMessage(sessionId, content);
}

void AppCoordinator::onReviewRequested()
{
    startReview();
}

void AppCoordinator::onReviewAnswered(const QString &sessionId, qint64 cardId, int ease)
{
    Q_UNUSED(sessionId);
    if (!m_dataSync)
        return;

    auto rating = static_cast<EaseRating>(qBound(1, ease, 4));
    m_dataSync->answerCard(cardId, rating);
}

void AppCoordinator::onSettingsChanged(const SRSConfig &config)
{
    if (!m_dataSync)
        return;

    m_config.srsConfig = config;
    m_dataSync->disconnectFromEngine();
    m_dataSync->connectToEngine(config);
    m_dataSync->fullSync();
    saveConfig();
}

// ── DataSync 状态处理 ──

void AppCoordinator::onDataSyncConnected()
{
    SPDLOG_LOGGER_INFO(logger(), "DataSync connected");
    if (m_ui) {
        m_ui->showStatus(QStringLiteral("Syncing..."));
        m_ui->appendSystemMessage(QStringLiteral("Anki connected. Starting sync..."));
    }

    if (m_dataSync)
        m_dataSync->fullSync();
}

void AppCoordinator::onDataSyncDisconnected()
{
    SPDLOG_LOGGER_INFO(logger(), "DataSync disconnected");
    if (m_ui) {
        m_ui->showStatus(QStringLiteral("Anki not connected"));
        m_ui->appendSystemMessage(QStringLiteral("Anki disconnected. Check if Anki is running with AnkiConnect plugin."));
    }
}

void AppCoordinator::onDataSyncError(const SyncError &error)
{
    SPDLOG_LOGGER_WARN(logger(), "DataSync error [{}]: {}", error.code.toStdString(), error.message.toStdString());
    if (m_ui) {
        m_ui->showStatus(QStringLiteral("Error: %1").arg(error.message));
        m_ui->appendSystemMessage(QStringLiteral("Sync error [%1]: %2").arg(error.code, error.message));
    }
}

// ── SceneOrchestrator 信号转发 / 处理 ──

void AppCoordinator::onSessionCreated(const QString &sessionId, const QString &openingMessage)
{
    SPDLOG_LOGGER_INFO(logger(), "session created {}", sessionId.toStdString());
    if (!m_ui)
        return;

    m_currentSessionId = sessionId;
    saveLastSession(sessionId);
    m_ui->showStatus(QStringLiteral("In session"));
    m_ui->showChatView(sessionId);
    m_ui->showMessage(sessionId, openingMessage);
}

void AppCoordinator::onResponseStreaming(const QString &sessionId, const QString &chunk)
{
    if (m_ui)
        m_ui->showMessage(sessionId, chunk);
}

void AppCoordinator::onResponseComplete(const QString &sessionId, const QString &message)
{
    Q_UNUSED(sessionId);
    Q_UNUSED(message);
    // 流式消息已逐 chunk 展示，complete 仅作标记，UI 可据此切换消息状态
}

void AppCoordinator::onSessionError(const QString &sessionId, const QString &error)
{
    SPDLOG_LOGGER_ERROR(logger(), "session error {}: {}", sessionId.toStdString(), error.toStdString());
    if (m_ui) {
        m_ui->showMessage(sessionId,
            QStringLiteral("[Error] %1").arg(error));
    }
}

void AppCoordinator::onSessionEnded(const QString &sessionId)
{
    SPDLOG_LOGGER_INFO(logger(), "session ended {}", sessionId.toStdString());
    if (sessionId == m_currentSessionId)
        m_currentSessionId.clear();

    if (m_ui) {
        m_ui->showSummary(sessionId);
        m_ui->setDueEntryCount(dueEntryCount());
    }
}

// ── 私有方法 ──

void AppCoordinator::createSubModules(const AppConfig &config)
{
    m_hold = new Hold(config.dataDir, this);

    auto *engine = new AnkiConnectEngine(this);
    m_dataSync = new DataSync(engine, m_hold, this);

    // DataSync 状态 → UI 反馈
    connect(m_dataSync, &IDataSync::connected,
            this, &AppCoordinator::onDataSyncConnected);
    connect(m_dataSync, &IDataSync::disconnected,
            this, &AppCoordinator::onDataSyncDisconnected);
    connect(m_dataSync, &IDataSync::errorOccurred,
            this, &AppCoordinator::onDataSyncError);
    connect(m_dataSync, &IDataSync::syncFinished, this, [this](const SyncReport &r) {
        if (m_ui) {
            m_ui->setDueEntryCount(dueEntryCount());
            if (r.errors.isEmpty()) {
                m_ui->showStatus(QStringLiteral("Ready (%1 entries, pulled %2)")
                    .arg(dueEntryCount()).arg(r.notesPulled));
            }
            // 若有错误，保留 onDataSyncError 设置的错误状态
        }
    });

    m_sceneOrchestrator = new SceneOrchestrator(this);
    m_sceneOrchestrator->configure(config.sceneConfig);
}

void AppCoordinator::connectUiSignals()
{
    if (!m_uiObject)
        return;

    // 使用 SIGNAL/SLOT 宏——因 IUIModule 已去除 QObject 继承，
    // 信号声明在 MainWindow（m_uiObject）上，需运行时字符串匹配
    connect(m_uiObject, SIGNAL(messageSent(QString,QString)),
            this, SLOT(onMessageSent(QString,QString)));
    connect(m_uiObject, SIGNAL(reviewRequested()),
            this, SLOT(onReviewRequested()));
    connect(m_uiObject, SIGNAL(reviewAnswered(QString,qint64,int)),
            this, SLOT(onReviewAnswered(QString,qint64,int)));
    connect(m_uiObject, SIGNAL(settingsChanged(SRSConfig)),
            this, SLOT(onSettingsChanged(SRSConfig)));
}

void AppCoordinator::connectSceneOrchestratorSignals()
{
    if (!m_sceneOrchestrator)
        return;

    connect(m_sceneOrchestrator, &ISceneOrchestrator::sessionCreated,
            this, &AppCoordinator::onSessionCreated);
    connect(m_sceneOrchestrator, &ISceneOrchestrator::responseStreaming,
            this, &AppCoordinator::onResponseStreaming);
    connect(m_sceneOrchestrator, &ISceneOrchestrator::responseComplete,
            this, &AppCoordinator::onResponseComplete);
    connect(m_sceneOrchestrator, &ISceneOrchestrator::sessionError,
            this, &AppCoordinator::onSessionError);
    connect(m_sceneOrchestrator, &ISceneOrchestrator::sessionEnded,
            this, &AppCoordinator::onSessionEnded);
}

void AppCoordinator::restoreLastSession()
{
    QString lastId = loadLastSession();
    if (lastId.isEmpty() || !m_ui)
        return;

    // 会话本身在 SceneOrchestrator 内存中，重启后不存在
    // 仅通知 UI 恢复上次会话标识，UI 据此展示历史会话列表
    if (m_ui)
        m_ui->showChatView(lastId);
}

// ── 持久化（通过 Hold） ──

void AppCoordinator::saveLastSession(const QString &sessionId)
{
    if (!m_hold)
        return;
    m_hold->save(kStateId, kLastSession, sessionId.toUtf8());
}

QString AppCoordinator::loadLastSession() const
{
    if (!m_hold)
        return {};
    QByteArray data = m_hold->load(kStateId, kLastSession);
    return QString::fromUtf8(data);
}

void AppCoordinator::saveConfig() const
{
    if (!m_hold)
        return;

    QJsonObject obj;
    obj[QStringLiteral("url")] = m_config.srsConfig.url;
    obj[QStringLiteral("deckName")] = m_config.srsConfig.deckName;
    obj[QStringLiteral("modelName")] = m_config.srsConfig.modelName;
    obj[QStringLiteral("apiVersion")] = m_config.srsConfig.apiVersion;
    obj[QStringLiteral("sceneType")] = static_cast<int>(m_config.defaultSceneType);
    obj[QStringLiteral("dueEntryLimit")] = m_config.dueEntryLimit;

    m_hold->save(kAppConfigId, kConfigName,
                 QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void AppCoordinator::loadConfig(AppConfig &config) const
{
    if (!m_hold)
        return;

    QByteArray data = m_hold->load(kAppConfigId, kConfigName);
    if (data.isEmpty())
        return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    config.srsConfig.url = obj.value(QStringLiteral("url")).toString(config.srsConfig.url);
    config.srsConfig.deckName = obj.value(QStringLiteral("deckName")).toString(config.srsConfig.deckName);
    config.srsConfig.modelName = obj.value(QStringLiteral("modelName")).toString(config.srsConfig.modelName);
    config.srsConfig.apiVersion = obj.value(QStringLiteral("apiVersion")).toInt(config.srsConfig.apiVersion);
    config.defaultSceneType = static_cast<SceneType>(
        obj.value(QStringLiteral("sceneType")).toInt(static_cast<int>(config.defaultSceneType)));
    config.dueEntryLimit = obj.value(QStringLiteral("dueEntryLimit")).toInt(config.dueEntryLimit);
}
