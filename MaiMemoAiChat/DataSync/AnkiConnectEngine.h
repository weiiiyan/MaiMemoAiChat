#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include "ISRSEngine.h"

class QNetworkReply;

/**
 * @brief AnkiConnect REST API 封装，实现 ISRSEngine 接口
 *
 * AnkiConnect 是 Anki 桌面插件，在 localhost:8765 提供 HTTP JSON API。
 * 本类封装 HTTP POST 请求构造、JSON 序列化/反序列化、错误处理。
 *
 * AnkiConnect API 格式:
 *   请求:  POST / { "action": "...", "version": 6, "params": {...} }
 *   响应:  { "result": ..., "error": null | "..." }
 */
class AnkiConnectEngine : public ISRSEngine
{
    Q_OBJECT

public:
    explicit AnkiConnectEngine(QObject *parent = nullptr);
    ~AnkiConnectEngine() override = default;

    void connectToEngine(const SRSConfig &config) override;
    void disconnectFromEngine() override;
    bool isConnected() const override;
    EngineType engineType() const override;
    bool isWritable() const override;

    void fetchDeckNames() override;
    void fetchModelNames() override;
    void fetchModelFieldNames(const QString &modelName) override;
    void fetchNoteIds(const QString &query) override;
    void fetchCardIds(const QString &query) override;
    void fetchNotesInfo(const QList<qint64> &noteIds) override;
    void fetchCardsInfo(const QList<qint64> &cardIds) override;
    void fetchModifiedNoteIds(qint64 since) override;

    void createNote(const QString &deckName, const QString &modelName,
                    const QMap<QString, QString> &fields,
                    const QStringList &tags) override;
    void updateNoteFields(qint64 noteId,
                          const QMap<QString, QString> &fields) override;
    void deleteNotes(const QList<qint64> &noteIds) override;

    void submitReviews(const QList<AnswerRecord> &answers) override;

private:
    /// 发送 AnkiConnect 请求，返回 QNetworkReply 指针（调用方负责连接信号）
    QNetworkReply *sendRequest(const QString &action, const QJsonObject &params);

    /// 检查响应是否有错误，有则 emit engineError 并返回 true
    bool handleResponseError(const QJsonObject &response);

    /// 从 cardsInfo API 响应的单个 card JSON 提取 MemEntry
    static MemEntry parseCardInfo(const QJsonObject &card);

    QNetworkAccessManager *m_nam;
    QString m_url;
    int m_apiVersion = 6;
    bool m_connected = false;
};
