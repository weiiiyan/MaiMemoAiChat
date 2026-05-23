#include "AppCoordinator.h"
#include "../Hold/Hold.h"
#include "../DataSync/IDataSync.h"
#include "../DataSync/DataSync.h"
#include "../DataSync/AnkiConnectEngine.h"
#include "../SceneOrchestrator/ISceneOrchestrator.h"
#include "../SceneOrchestrator/SceneOrchestrator.h"

#include <QJsonDocument>
#include <QJsonObject>

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

    // 1. 加载持久化的配置（覆盖默认值）
    loadConfig(m_config);

    // 2. 创建子模块（Hold → DataSync → SceneOrchestrator）
    createSubModules(m_config);

    // 3. 连接 IUIModule 信号 → AppCoordinator 内部 slot
    connectUiSignals();

    // 4. 连接 SceneOrchestrator 信号 → 转发到 UI
    connectSceneOrchestratorSignals();

    // 5. 连接 SRS 引擎并全量同步（异步，不阻塞 initialize 返回）
    m_dataSync->connectToEngine(m_config.srsConfig);
    m_dataSync->fullSync();

    // 6. 恢复上次会话状态
    restoreLastSession();

    m_initialized = true;

    if (m_ui) {
        m_ui->setInitialized(true);
        m_ui->setDueEntryCount(dueEntryCount());
    }

    return true;
}

void AppCoordinator::shutdown()
{
    if (!m_initialized)
        return;

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

    if (m_ui)
        m_ui->setInitialized(false);
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

    // 已有活跃会话则先结束
    if (!m_currentSessionId.isEmpty()) {
        m_sceneOrchestrator->endSession(m_currentSessionId);
        m_currentSessionId.clear();
    }

    QList<MemEntry> dueEntries = m_dataSync->getDueEntries(m_config.dueEntryLimit);
    if (dueEntries.isEmpty())
        return;

    QString sessionId = m_sceneOrchestrator->createSession(
        dueEntries, m_config.defaultSceneType, m_config.sceneConfig);

    m_currentSessionId = sessionId;
    saveLastSession(sessionId);
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

// ── SceneOrchestrator 信号转发 / 处理 ──

void AppCoordinator::onSessionCreated(const QString &sessionId, const QString &openingMessage)
{
    if (!m_ui)
        return;

    m_currentSessionId = sessionId;
    saveLastSession(sessionId);
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
    if (m_ui) {
        m_ui->showMessage(sessionId,
            QStringLiteral("[Error] %1").arg(error));
    }
}

void AppCoordinator::onSessionEnded(const QString &sessionId)
{
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
    // Hold: 数据根目录
    m_hold = new Hold(config.dataDir, this);

    // DataSync: AnkiConnect 引擎 + Hold 缓存
    auto *engine = new AnkiConnectEngine(this);
    m_dataSync = new DataSync(engine, m_hold, this);

    // 监听 DataSync 同步完成 → 刷新 UI 条目数
    connect(m_dataSync, &IDataSync::syncFinished, this, [this](const SyncReport &) {
        if (m_ui)
            m_ui->setDueEntryCount(dueEntryCount());
    });

    // SceneOrchestrator: AI 交互管理
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
