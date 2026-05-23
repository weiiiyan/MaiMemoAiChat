#include "SceneOrchestrator.h"
#include "IAIManager.h"
#include "WenXinAiManager.h"
#include "QianWenAiManager.h"
#include <QUuid>
#include <QDateTime>

// ── System Prompt 模板 ──

static const char *kReadingPrompt = R"(
You are an English reading guide. Your task is to help the user practice English reading comprehension.

Target vocabulary for this session:
%1

Rules:
1. Generate ONE simple English sentence at a time, naturally using one or more words from the target vocabulary list above.
2. Guide the user to imagine the scene directly in English — discourage translation into Chinese.
3. After showing the sentence, ask a question in English to check the user's understanding.
4. When the user responds, give brief, encouraging feedback. Correct any misunderstandings gently, then move on.
5. Keep your sentences short and natural. Gradually increase complexity as the session progresses.
6. Stay in English throughout. Do not speak Chinese unless the user is clearly lost and asks for help.

Now begin: present your first sentence and question to the user.
)";

static const char *kWritingPrompt = R"(
You are an English writing coach. Your task is to help the user practice writing English sentences.

Target vocabulary for this session:
%1

Rules:
1. Each round, describe a simple scene or situation in Chinese. The scene should naturally call for one or more words from the target vocabulary list.
2. Wait for the user to write an English sentence expressing that scene.
3. When the user responds, check for: grammar mistakes, word choice, sentence structure, and whether the target vocabulary was used correctly.
4. ALWAYS provide a natural reference sentence in English, then explain any key errors in Chinese.
5. Be specific and precise in your corrections, but always encouraging.
6. Use the target vocabulary naturally — do not force every word into every scene.

Now begin: describe your first scene in Chinese and ask the user to write the English sentence.
)";

static const char *kListeningPrompt = R"(
You are an English listening practice partner. Your task is to help the user improve English listening comprehension.

Target vocabulary for this session:
%1

Rules:
1. Each round, present ONE English sentence as text (audio is not available in this session, so the user will read and imagine hearing it).
2. After presenting the sentence, ask the user to describe what they understood — the main idea, key details, or any specific words.
3. Verify the user's understanding. If they misunderstood something, clarify and explain.
4. Gradually increase sentence difficulty and length as the user improves.
5. Naturally incorporate the target vocabulary into your sentences.
6. Keep the tone warm and supportive. Celebrate correct understanding.

Now begin: present your first sentence and ask the user what they understood.
)";

static const char *kSpeakingPrompt = R"(
You are an English speaking coach. Your task is to help the user practice spoken English.

Target vocabulary for this session:
%1

Rules:
1. Each round, describe a brief scenario or situation in English.
2. Ask the user to say aloud the English sentence(s) they would use in that situation.
3. After the user responds (they will type what they said), give feedback on:
   - Naturalness and fluency of expression
   - Word choice and phrasing
   - Pronunciation tips (note common pitfalls for Chinese speakers, e.g., th-sounds, word stress, linking)
4. Point out which target vocabulary words fit the scenario and how to use them naturally.
5. Be highly encouraging — confidence is key for speaking practice. Celebrate every attempt.
6. Stay in English unless explaining a complex pronunciation point that needs Chinese.

Now begin: describe your first scenario and ask the user to speak their response.
)";

// ── SceneOrchestrator ──

SceneOrchestrator::SceneOrchestrator(QObject *parent)
    : ISceneOrchestrator(parent)
{
}

void SceneOrchestrator::configure(const SceneConfig &config)
{
    // 全局配置暂存，真正创建会话时按会话级 config 创建 AI manager
    Q_UNUSED(config);
}

QString SceneOrchestrator::createSession(const QList<MemEntry> &entries,
                                          SceneType sceneType,
                                          const SceneConfig &config)
{
    const QString sessionId = generateSessionId();

    // 构建 system prompt
    QString systemPrompt = buildSystemPrompt(sceneType, entries);

    // 创建会话
    SceneSession session;
    session.id = sessionId;
    session.type = sceneType;
    session.entries = entries;
    session.systemPrompt = systemPrompt;
    session.roundCount = 0;
    session.status = SessionStatus::Creating;
    session.createdAt = QDateTime::currentSecsSinceEpoch();

    m_sessions.insert(sessionId, session);

    // 创建并配置 AI 管理器
    IAIManager *ai = createAiManager(config, this);
    ai->configure(config);
    m_aiManagers.insert(sessionId, ai);
    m_managerToSession.insert(ai, sessionId);

    m_currentSessionId = sessionId;

    // 连接 AI 信号
    connect(ai, &IAIManager::responseStreaming,
            this, &SceneOrchestrator::onAiResponseStreaming);
    connect(ai, &IAIManager::responseComplete,
            this, &SceneOrchestrator::onAiResponseComplete);
    connect(ai, &IAIManager::responseError,
            this, &SceneOrchestrator::onAiResponseError);

    // 发送初始消息（system prompt + 启动指令），获取 AI 开场白
    QList<ChatMessage> messages;
    ChatMessage sysMsg;
    sysMsg.role = QStringLiteral("system");
    sysMsg.content = systemPrompt;
    messages.append(sysMsg);

    ChatMessage userMsg;
    userMsg.role = QStringLiteral("user");
    userMsg.content = QStringLiteral("Hello! Let's begin our practice session. Please start.");
    messages.append(userMsg);

    // 记录到会话历史
    m_sessions[sessionId].history = messages;

    ai->sendMessage(messages);

    return sessionId;
}

void SceneOrchestrator::sendMessage(const QString &sessionId, const QString &content)
{
    if (!m_sessions.contains(sessionId))
        return;

    SceneSession &session = m_sessions[sessionId];
    if (session.status != SessionStatus::Active)
        return;

    IAIManager *ai = m_aiManagers.value(sessionId);
    if (!ai || ai->isBusy())
        return;

    // 追加用户消息到历史
    ChatMessage userMsg;
    userMsg.role = QStringLiteral("user");
    userMsg.content = content;
    session.history.append(userMsg);
    session.roundCount++;

    ai->sendMessage(session.history);
}

void SceneOrchestrator::stopResponse(const QString &sessionId)
{
    IAIManager *ai = m_aiManagers.value(sessionId);
    if (ai)
        ai->stopResponse();
}

void SceneOrchestrator::endSession(const QString &sessionId)
{
    if (!m_sessions.contains(sessionId))
        return;

    // 停止 AI 并清理
    IAIManager *ai = m_aiManagers.take(sessionId);
    if (ai) {
        m_managerToSession.remove(ai);
        ai->stopResponse();
        ai->deleteLater();
    }

    m_sessions[sessionId].status = SessionStatus::Ended;

    emit sessionEnded(sessionId);
}

SceneSession SceneOrchestrator::getSession(const QString &sessionId) const
{
    return m_sessions.value(sessionId);
}

// ── Private Slots ──

void SceneOrchestrator::onAiResponseStreaming(const QString &chunk)
{
    QString sid = sessionIdForSender();
    if (!sid.isEmpty())
        emit responseStreaming(sid, chunk);
}

void SceneOrchestrator::onAiResponseComplete(const QString &fullMessage)
{
    QString sid = sessionIdForSender();
    if (sid.isEmpty())
        return;

    SceneSession &session = m_sessions[sid];

    // 追加 AI 回复到历史
    ChatMessage aiMsg;
    aiMsg.role = QStringLiteral("assistant");
    aiMsg.content = fullMessage;
    session.history.append(aiMsg);

    if (session.status == SessionStatus::Creating) {
        // 首次 AI 回复 = 开场白，会话正式激活
        session.status = SessionStatus::Active;
        emit sessionCreated(sid, fullMessage);
    } else {
        emit responseComplete(sid, fullMessage);
    }
}

void SceneOrchestrator::onAiResponseError(const QString &error)
{
    QString sid = sessionIdForSender();
    if (!sid.isEmpty())
        emit sessionError(sid, error);
}

// ── Private Helpers ──

IAIManager *SceneOrchestrator::createAiManager(const SceneConfig &config, QObject *parent)
{
    switch (config.aiProvider) {
    case AiProvider::WenXin:
        return new WenXinAiManager(parent);
    case AiProvider::QianWen:
    default:
        return new QianWenAiManager(parent);
    }
}

QString SceneOrchestrator::buildSystemPrompt(SceneType type, const QList<MemEntry> &entries)
{
    // 提取词汇列表
    QStringList words;
    for (const auto &e : entries) {
        // 优先取字段映射中的 "word" 字段，否则取首个字段
        QString word = e.fields.value(QStringLiteral("word"));
        if (word.isEmpty() && !e.fields.isEmpty())
            word = e.fields.first();
        if (!word.isEmpty())
            words.append(QStringLiteral("- %1").arg(word));
    }
    QString wordList = words.isEmpty()
        ? QStringLiteral("(no specific vocabulary — use general English practice)")
        : words.join(QStringLiteral("\n"));

    const char *tmpl = kReadingPrompt;
    switch (type) {
    case SceneType::Writing:   tmpl = kWritingPrompt;   break;
    case SceneType::Listening: tmpl = kListeningPrompt;  break;
    case SceneType::Speaking:  tmpl = kSpeakingPrompt;   break;
    case SceneType::Reading:
    default:                   tmpl = kReadingPrompt;    break;
    }

    return QString::fromUtf8(tmpl).arg(wordList);
}

QString SceneOrchestrator::generateSessionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SceneOrchestrator::sessionIdForSender() const
{
    auto *ai = qobject_cast<IAIManager *>(sender());
    if (!ai)
        return {};
    return m_managerToSession.value(ai);
}
