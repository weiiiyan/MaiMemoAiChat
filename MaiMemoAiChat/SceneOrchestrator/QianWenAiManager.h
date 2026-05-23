#pragma once

#include "IAIManager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

/**
 * @brief 阿里通义千问（DashScope）API 封装，兼容 OpenAI 格式
 *
 * 认证: API Key 通过 Authorization: Bearer 头传递
 * 端点: POST https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
 *
 * API 文档: https://help.aliyun.com/zh/model-studio/developer-reference/use-qwen-by-calling-api
 */
class QianWenAiManager : public IAIManager
{
    Q_OBJECT

public:
    explicit QianWenAiManager(QObject *parent = nullptr);
    ~QianWenAiManager() override = default;

    void configure(const SceneConfig &config) override;
    void sendMessage(const QList<ChatMessage> &messages) override;
    void stopResponse() override;
    bool isBusy() const override;

private slots:
    void onChatReplyFinished();
    void onChatReadyRead();
    void onChatErrorOccurred(QNetworkReply::NetworkError code);

private:
    void doSendMessage(const QList<ChatMessage> &messages);
    QJsonObject buildRequestBody(const QList<ChatMessage> &messages,
                                 const SceneConfig &config);

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_activeReply = nullptr;
    QString m_model;
    SceneConfig m_config;
    bool m_busy = false;

    /// SSE 流式解析缓冲
    QByteArray m_streamBuffer;
    QString m_fullResponse;
};
