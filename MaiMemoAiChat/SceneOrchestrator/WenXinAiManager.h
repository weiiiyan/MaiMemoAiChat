#pragma once

#include "IAIManager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

/**
 * @brief 百度文心一言（ERNIE-Bot）API 封装
 *
 * 认证流程:
 *   1. POST https://aip.baidubce.com/oauth/2.0/token
 *      获取 access_token（有效期约 30 天）
 *   2. POST https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/{model}?access_token={token}
 *      发送对话请求，支持 SSE 流式响应
 *
 * API 文档: https://cloud.baidu.com/doc/WENXINWORKSHOP/s/flfmc9do2
 */
class WenXinAiManager : public IAIManager
{
    Q_OBJECT

public:
    explicit WenXinAiManager(QObject *parent = nullptr);
    ~WenXinAiManager() override = default;

    void configure(const SceneConfig &config) override;
    void sendMessage(const QList<ChatMessage> &messages) override;
    void stopResponse() override;
    bool isBusy() const override;

private slots:
    void onTokenReplyFinished();
    void onChatReplyFinished();
    void onChatReadyRead();
    void onChatErrorOccurred(QNetworkReply::NetworkError code);

private:
    void fetchAccessToken();
    void doSendMessage(const QList<ChatMessage> &messages);
    QJsonObject buildRequestBody(const QList<ChatMessage> &messages,
                                 const SceneConfig &config);

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_activeReply = nullptr;
    QString m_accessToken;
    QString m_model;
    SceneConfig m_config;
    bool m_busy = false;

    /// SSE 流式解析缓冲
    QByteArray m_streamBuffer;
    QString m_fullResponse;
};
