#include "QianWenAiManager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <spdlog/spdlog.h>

QianWenAiManager::QianWenAiManager(QObject *parent)
    : IAIManager(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void QianWenAiManager::configure(const SceneConfig &config)
{
    m_config = config;
    m_model = config.model.isEmpty()
        ? QStringLiteral("qwen-max")
        : config.model;
    SPDLOG_INFO("QianWenAiManager configured: model={}", m_model.toStdString());
}

void QianWenAiManager::sendMessage(const QList<ChatMessage> &messages)
{
    if (m_config.apiKey.isEmpty()) {
        SPDLOG_WARN("QianWenAiManager sendMessage: API key not configured");
        emit responseError(QStringLiteral("通义千问 API Key 未配置"));
        return;
    }
    doSendMessage(messages);
}

void QianWenAiManager::doSendMessage(const QList<ChatMessage> &messages)
{
    if (m_busy)
        return;

    m_busy = true;
    m_streamBuffer.clear();
    m_fullResponse.clear();

    SPDLOG_DEBUG("QianWenAiManager sending {} messages to model={}", messages.size(), m_model.toStdString());

    QJsonObject body = buildRequestBody(messages, m_config);

    QUrl url(QStringLiteral("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"));

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_config.apiKey).toUtf8());

    m_activeReply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_activeReply, &QNetworkReply::readyRead,
            this, &QianWenAiManager::onChatReadyRead);
    connect(m_activeReply, &QNetworkReply::finished,
            this, &QianWenAiManager::onChatReplyFinished);
    connect(m_activeReply, &QNetworkReply::errorOccurred,
            this, &QianWenAiManager::onChatErrorOccurred);
}

QJsonObject QianWenAiManager::buildRequestBody(const QList<ChatMessage> &messages,
                                                const SceneConfig &config)
{
    QJsonArray msgs;
    for (const auto &m : messages) {
        QJsonObject msg;
        msg[QStringLiteral("role")] = m.role;
        msg[QStringLiteral("content")] = m.content;
        msgs.append(msg);
    }

    QJsonObject body;
    body[QStringLiteral("model")] = m_model;
    body[QStringLiteral("messages")] = msgs;
    body[QStringLiteral("stream")] = true;
    body[QStringLiteral("temperature")] = config.temperature;
    body[QStringLiteral("max_tokens")] = config.maxTokens;
    if (config.topP > 0)
        body[QStringLiteral("top_p")] = config.topP;

    QJsonObject streamOptions;
    streamOptions[QStringLiteral("include_usage")] = true;
    body[QStringLiteral("stream_options")] = streamOptions;

    return body;
}

void QianWenAiManager::onChatReadyRead()
{
    if (!m_activeReply)
        return;

    // 千问 SSE 格式: data: {...}\n\n
    m_streamBuffer.append(m_activeReply->readAll());

    while (true) {
        int idx = m_streamBuffer.indexOf("\n\n");
        if (idx < 0)
            break;

        QByteArray line = m_streamBuffer.left(idx).trimmed();
        m_streamBuffer.remove(0, idx + 2);

        if (line.isEmpty() || !line.startsWith("data: "))
            continue;

        QByteArray jsonData = line.mid(6);  // 去掉 "data: "
        if (jsonData == "[DONE]")
            continue;

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull())
            continue;

        QJsonObject obj = doc.object();
        QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty())
            continue;

        QJsonObject choice = choices.first().toObject();
        QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
        QString content = delta.value(QStringLiteral("content")).toString();

        if (!content.isEmpty()) {
            m_fullResponse += content;
            emit responseStreaming(content);
        }
    }
}

void QianWenAiManager::onChatReplyFinished()
{
    if (!m_activeReply)
        return;

    // 处理缓冲中剩余的数据
    onChatReadyRead();

    m_activeReply->deleteLater();
    m_activeReply = nullptr;
    m_busy = false;

    if (m_fullResponse.isEmpty()) {
        SPDLOG_WARN("QianWenAiManager response empty");
        emit responseError(QStringLiteral("通义千问返回空内容"));
    } else {
        SPDLOG_DEBUG("QianWenAiManager response complete: {} chars", m_fullResponse.size());
        emit responseComplete(m_fullResponse);
    }
}

void QianWenAiManager::onChatErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if (!m_activeReply)
        return;

    QString err = m_activeReply->errorString();
    SPDLOG_ERROR("QianWenAiManager chat error: {}", err.toStdString());
    m_activeReply->deleteLater();
    m_activeReply = nullptr;
    m_busy = false;
    emit responseError(QStringLiteral("通义千问请求失败: %1").arg(err));
}

void QianWenAiManager::stopResponse()
{
    if (m_activeReply) {
        SPDLOG_DEBUG("QianWenAiManager stopping response");
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
    }
    m_busy = false;
}

bool QianWenAiManager::isBusy() const
{
    return m_busy;
}
