#include "AnkiConnectEngine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

AnkiConnectEngine::AnkiConnectEngine(QObject *parent)
    : ISRSEngine(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void AnkiConnectEngine::connectToEngine(const SRSConfig &config)
{
    m_url = config.url;
    m_apiVersion = config.apiVersion;
    // 调用 version API 验证 AnkiConnect 是否可用
    QNetworkReply *reply = sendRequest(QStringLiteral("version"), QJsonObject());
    reply->setProperty("action", QStringLiteral("version"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp))
            return;
        m_connected = (resp.value("result").toInt() >= 4); // AnkiConnect v4+
        if (m_connected)
            emit connected();
        else
            emit engineError(QStringLiteral("AnkiConnect version too old or unavailable"));
    });
}

void AnkiConnectEngine::disconnectFromEngine()
{
    m_connected = false;
    emit disconnected(QStringLiteral("User disconnected"));
}

bool AnkiConnectEngine::isConnected() const
{
    return m_connected;
}

EngineType AnkiConnectEngine::engineType() const
{
    return EngineType::AnkiConnect;
}

bool AnkiConnectEngine::isWritable() const
{
    return true; // AnkiConnect 支持完整读写
}

// ── 牌组/模型发现 ──

void AnkiConnectEngine::fetchDeckNames()
{
    QNetworkReply *reply = sendRequest(QStringLiteral("deckNames"), QJsonObject());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QStringList names;
        for (const QJsonValue &v : resp.value("result").toArray())
            names.append(v.toString());
        emit deckNamesReady(names);
    });
}

void AnkiConnectEngine::fetchModelNames()
{
    QNetworkReply *reply = sendRequest(QStringLiteral("modelNames"), QJsonObject());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QStringList names;
        for (const QJsonValue &v : resp.value("result").toArray())
            names.append(v.toString());
        emit modelNamesReady(names);
    });
}

void AnkiConnectEngine::fetchModelFieldNames(const QString &modelName)
{
    QJsonObject params;
    params["modelName"] = modelName;
    QNetworkReply *reply = sendRequest(QStringLiteral("modelFieldNames"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QStringList fields;
        for (const QJsonValue &v : resp.value("result").toArray())
            fields.append(v.toString());
        emit modelFieldNamesReady(fields);
    });
}

// ── 数据抓取 ──

void AnkiConnectEngine::fetchNoteIds(const QString &query)
{
    QJsonObject params;
    params["query"] = query;
    QNetworkReply *reply = sendRequest(QStringLiteral("findNotes"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QList<qint64> ids;
        for (const QJsonValue &v : resp.value("result").toArray())
            ids.append(static_cast<qint64>(v.toDouble()));
        emit noteIdsReady(ids);
    });
}

void AnkiConnectEngine::fetchCardIds(const QString &query)
{
    QJsonObject params;
    params["query"] = query;
    QNetworkReply *reply = sendRequest(QStringLiteral("findCards"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QList<qint64> ids;
        for (const QJsonValue &v : resp.value("result").toArray())
            ids.append(static_cast<qint64>(v.toDouble()));
        emit cardIdsReady(ids);
    });
}

void AnkiConnectEngine::fetchNotesInfo(const QList<qint64> &noteIds)
{
    QJsonArray arr;
    for (qint64 id : noteIds)
        arr.append(id);
    QJsonObject params;
    params["notes"] = arr;
    QNetworkReply *reply = sendRequest(QStringLiteral("notesInfo"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QList<MemEntry> entries;
        for (const QJsonValue &v : resp.value("result").toArray()) {
            QJsonObject note = v.toObject();
            MemEntry entry;
            entry.noteId = static_cast<qint64>(note.value("noteId").toDouble());
            entry.modelName = note.value("modelName").toString();
            for (const QJsonValue &t : note.value("tags").toArray())
                entry.tags.append(t.toString());
            // fields: { "Front": { "value": "...", "order": 0 }, ... }
            QJsonObject fieldsObj = note.value("fields").toObject();
            for (auto it = fieldsObj.begin(); it != fieldsObj.end(); ++it)
                entry.fields[it.key()] = it.value().toObject().value("value").toString();
            entries.append(entry);
        }
        emit notesInfoReady(entries);
    });
}

void AnkiConnectEngine::fetchCardsInfo(const QList<qint64> &cardIds)
{
    QJsonArray arr;
    for (qint64 id : cardIds)
        arr.append(id);
    QJsonObject params;
    params["cards"] = arr;
    QNetworkReply *reply = sendRequest(QStringLiteral("cardsInfo"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        QList<MemEntry> entries;
        for (const QJsonValue &v : resp.value("result").toArray())
            entries.append(parseCardInfo(v.toObject()));
        emit cardsInfoReady(entries);
    });
}

void AnkiConnectEngine::fetchModifiedNoteIds(qint64 since)
{
    QJsonObject params;
    params["modTime"] = since;
    QNetworkReply *reply = sendRequest(QStringLiteral("notesModTime"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        // notesModTime 返回 [{noteId: ..., mod: ...}, ...]
        QList<qint64> ids;
        for (const QJsonValue &v : resp.value("result").toArray())
            ids.append(static_cast<qint64>(v.toObject().value("noteId").toDouble()));
        emit modifiedNoteIdsReady(ids);
    });
}

// ── 笔记 CRUD ──

void AnkiConnectEngine::createNote(const QString &deckName, const QString &modelName,
                                    const QMap<QString, QString> &fields,
                                    const QStringList &tags)
{
    QJsonObject noteObj;
    noteObj["deckName"] = deckName;
    noteObj["modelName"] = modelName;
    QJsonObject fieldsObj;
    for (auto it = fields.begin(); it != fields.end(); ++it)
        fieldsObj[it.key()] = it.value();
    noteObj["fields"] = fieldsObj;
    QJsonArray tagsArr;
    for (const QString &t : tags)
        tagsArr.append(t);
    noteObj["tags"] = tagsArr;

    QJsonObject params;
    params["note"] = noteObj;
    QNetworkReply *reply = sendRequest(QStringLiteral("addNote"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        qint64 noteId = static_cast<qint64>(resp.value("result").toDouble());
        emit noteCreated(noteId);
    });
}

void AnkiConnectEngine::updateNoteFields(qint64 noteId,
                                          const QMap<QString, QString> &fields)
{
    QJsonObject fieldsObj;
    for (auto it = fields.begin(); it != fields.end(); ++it)
        fieldsObj[it.key()] = it.value();

    QJsonObject params;
    QJsonObject noteObj;
    noteObj["id"] = noteId;
    noteObj["fields"] = fieldsObj;
    params["note"] = noteObj;
    QNetworkReply *reply = sendRequest(QStringLiteral("updateNoteFields"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply, noteId]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        emit noteFieldsUpdated(noteId);
    });
}

void AnkiConnectEngine::deleteNotes(const QList<qint64> &noteIds)
{
    QJsonArray arr;
    for (qint64 id : noteIds)
        arr.append(id);
    QJsonObject params;
    params["notes"] = arr;
    QNetworkReply *reply = sendRequest(QStringLiteral("deleteNotes"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply, noteIds]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        emit notesDeleted(noteIds);
    });
}

// ── 复习提交 ──

void AnkiConnectEngine::submitReviews(const QList<AnswerRecord> &answers)
{
    QJsonArray arr;
    for (const AnswerRecord &a : answers) {
        QJsonObject obj;
        obj["cardId"] = a.cardId;
        obj["ease"] = static_cast<int>(a.ease);
        arr.append(obj);
    }
    QJsonObject params;
    params["answers"] = arr;
    QNetworkReply *reply = sendRequest(QStringLiteral("answerCards"), params);
    connect(reply, &QNetworkReply::finished, this, [this, reply, answers]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object();
        if (handleResponseError(resp)) return;
        emit reviewsSubmitted(answers.size());
    });
}

// ── 内部辅助 ──

QNetworkReply *AnkiConnectEngine::sendRequest(const QString &action,
                                               const QJsonObject &params)
{
    QJsonObject body;
    body["action"] = action;
    body["version"] = m_apiVersion;
    if (!params.isEmpty())
        body["params"] = params;

    QNetworkRequest req{QUrl(m_url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setTransferTimeout(10000); // 10s 超时，避免永久挂起
    QNetworkReply *reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // 网络层错误转发为 engineError，避免调用方永久等待
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError) {
        if (!reply->errorString().isEmpty())
            emit engineError(QStringLiteral("[%1] %2").arg(reply->url().toString(), reply->errorString()));
    });

    return reply;
}

bool AnkiConnectEngine::handleResponseError(const QJsonObject &response)
{
    if (response.contains("error") && !response.value("error").isNull()) {
        QString err = response.value("error").toString();
        if (!err.isEmpty())
            emit engineError(err);
        return true;
    }
    return false;
}

MemEntry AnkiConnectEngine::parseCardInfo(const QJsonObject &card)
{
    MemEntry entry;
    entry.cardId = static_cast<qint64>(card.value("cardId").toDouble());
    entry.noteId = static_cast<qint64>(card.value("note").toDouble());
    entry.deckName = card.value("deckName").toString();
    entry.modelName = card.value("modelName").toString();
    entry.queue = static_cast<CardQueue>(card.value("queue").toInt(0));
    entry.due = static_cast<qint64>(card.value("due").toDouble());
    entry.intervalDays = static_cast<float>(card.value("interval").toDouble());
    // Anki 内 ease factor 存储为 2500 = 250%
    entry.easeFactor = static_cast<float>(card.value("factor").toDouble() / 1000.0);
    entry.reps = card.value("reps").toInt(0);
    entry.lapses = card.value("lapses").toInt(0);
    // fields 也可能包含在 cardsInfo 中
    QJsonObject fieldsObj = card.value("fields").toObject();
    for (auto it = fieldsObj.begin(); it != fieldsObj.end(); ++it)
        entry.fields[it.key()] = it.value().toObject().value("value").toString();
    return entry;
}
