#include "WenXinAiManager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

WenXinAiManager::WenXinAiManager(QObject *parent)
    : IAIManager(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void WenXinAiManager::configure(const SceneConfig &config)
{
    m_config = config;
    m_model = config.model.isEmpty()
        ? QStringLiteral("ernie-4.0-turbo-128k")
        : config.model;

    fetchAccessToken();
}

void WenXinAiManager::fetchAccessToken()
{
    QUrl url(QStringLiteral("https://aip.baidubce.com/oauth/2.0/token"));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    params.addQueryItem(QStringLiteral("client_id"), m_config.apiKey);
    params.addQueryItem(QStringLiteral("client_secret"), m_config.secretKey);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply *reply = m_nam->post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &WenXinAiManager::onTokenReplyFinished);
}

void WenXinAiManager::onTokenReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit responseError(QStringLiteral("获取文心 access_token 失败: %1").arg(reply->errorString()));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains(QStringLiteral("error_description"))) {
        emit responseError(QStringLiteral("获取文心 access_token 失败: %1")
                               .arg(obj.value(QStringLiteral("error_description")).toString()));
        return;
    }

    m_accessToken = obj.value(QStringLiteral("access_token")).toString();
}

void WenXinAiManager::sendMessage(const QList<ChatMessage> &messages)
{
    if (m_accessToken.isEmpty()) {
        // token 尚未获取，延迟发送
        QMetaObject::Connection *conn = new QMetaObject::Connection;
        *conn = connect(this, &WenXinAiManager::responseError, this,
                        [conn]() { disconnect(*conn); delete conn; });
        emit responseError(QStringLiteral("文心一言未就绪，请稍后重试"));
        return;
    }
    doSendMessage(messages);
}

void WenXinAiManager::doSendMessage(const QList<ChatMessage> &messages)
{
    if (m_busy)
        return;

    m_busy = true;
    m_streamBuffer.clear();
    m_fullResponse.clear();

    QJsonObject body = buildRequestBody(messages, m_config);

    QUrl url(QStringLiteral("https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/%1")
                 .arg(m_model));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("access_token"), m_accessToken);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));

    m_activeReply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_activeReply, &QNetworkReply::readyRead,
            this, &WenXinAiManager::onChatReadyRead);
    connect(m_activeReply, &QNetworkReply::finished,
            this, &WenXinAiManager::onChatReplyFinished);
    connect(m_activeReply, &QNetworkReply::errorOccurred,
            this, &WenXinAiManager::onChatErrorOccurred);
}

QJsonObject WenXinAiManager::buildRequestBody(const QList<ChatMessage> &messages,
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
    body[QStringLiteral("messages")] = msgs;
    body[QStringLiteral("stream")] = true;
    body[QStringLiteral("temperature")] = config.temperature;
    body[QStringLiteral("max_output_tokens")] = config.maxTokens;
    if (config.topP > 0)
        body[QStringLiteral("top_p")] = config.topP;

    return body;
}

void WenXinAiManager::onChatReadyRead()
{
    if (!m_activeReply)
        return;

    // 文心一言 SSE 格式: data: {...}\n\n
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
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull())
            continue;

        QJsonObject obj = doc.object();
        bool isEnd = obj.value(QStringLiteral("is_end")).toBool();
        QString result = obj.value(QStringLiteral("result")).toString();

        if (!result.isEmpty()) {
            m_fullResponse += result;
            emit responseStreaming(result);
        }

        if (isEnd) {
            // is_end 会在一段完整 data 后出现，finished 信号会处理最终完成
        }
    }
}

void WenXinAiManager::onChatReplyFinished()
{
    if (!m_activeReply)
        return;

    m_activeReply->deleteLater();
    m_activeReply = nullptr;
    m_busy = false;

    if (m_fullResponse.isEmpty()) {
        emit responseError(QStringLiteral("文心一言返回空内容"));
    } else {
        emit responseComplete(m_fullResponse);
    }
}

void WenXinAiManager::onChatErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if (!m_activeReply)
        return;

    QString err = m_activeReply->errorString();
    m_activeReply->deleteLater();
    m_activeReply = nullptr;
    m_busy = false;
    emit responseError(QStringLiteral("文心一言请求失败: %1").arg(err));
}

void WenXinAiManager::stopResponse()
{
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
    }
    m_busy = false;
}

bool WenXinAiManager::isBusy() const
{
    return m_busy;
}
