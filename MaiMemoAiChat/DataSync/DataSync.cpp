#include "DataSync.h"
#include "ISRSEngine.h"
#include "../Hold/Hold.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include <spdlog/spdlog.h>

DataSync::DataSync(ISRSEngine *engine, Hold *hold, QObject *parent)
    : IDataSync(parent)
    , m_engine(engine)
    , m_hold(hold)
{
    // 转发引擎生命周期信号
    connect(m_engine, &ISRSEngine::connected, this, &IDataSync::connected);
    connect(m_engine, &ISRSEngine::disconnected, this, &IDataSync::disconnected);
    connect(m_engine, &ISRSEngine::engineError, this, [this](const QString &msg) {
        errorOccurred({QStringLiteral("engine"), msg});
        // 引擎错误若发生在同步过程中，需终止同步以防调用方永远等待
        if (m_syncCtx && m_syncCtx->step != SyncStep::Idle) {
            finishSyncWithError(msg);
        }
    });

    // 转发发现类信号
    connect(m_engine, &ISRSEngine::deckNamesReady, this, &IDataSync::deckNamesReady);
    connect(m_engine, &ISRSEngine::modelNamesReady, this, &IDataSync::modelNamesReady);
    connect(m_engine, &ISRSEngine::modelFieldNamesReady, this, &IDataSync::modelFieldNamesReady);

    // 转发 CRUD 结果信号
    connect(m_engine, &ISRSEngine::noteCreated, this, [this](qint64 noteId) {
        // 添加成功后拉取新笔记的完整信息以更新缓存
        pullFromEngine();
        noteAdded(noteId);
    });
    connect(m_engine, &ISRSEngine::noteFieldsUpdated, this, [this](qint64 noteId) {
        pullFromEngine();
        noteFieldsUpdated(noteId);
    });
    connect(m_engine, &ISRSEngine::notesDeleted, this, [this](const QList<qint64> &ids) {
        for (qint64 id : ids) {
            m_entriesByNoteId.remove(id);
            for (auto it = m_noteIdByCardId.begin(); it != m_noteIdByCardId.end();) {
                if (it.value() == id)
                    it = m_noteIdByCardId.erase(it);
                else
                    ++it;
            }
            noteDeleted(id);
        }
        saveCacheToHold();
        emit entriesChanged(ids);
    });

    // 同步状态机 —— 构造时连接，通过 m_syncCtx->step 分发
    connect(m_engine, &ISRSEngine::noteIdsReady, this, &DataSync::onSyncNoteIdsReady);
    connect(m_engine, &ISRSEngine::cardIdsReady, this, &DataSync::onSyncCardIdsReady);
    connect(m_engine, &ISRSEngine::notesInfoReady, this, &DataSync::onSyncNotesInfoReady);
    connect(m_engine, &ISRSEngine::cardsInfoReady, this, &DataSync::onSyncCardsInfoReady);
}

// ── 引擎生命周期 ──

void DataSync::connectToEngine(const SRSConfig &config)
{
    m_config = config;
    SPDLOG_INFO("DataSync connecting to engine: deck={}, url={}", config.deckName.toStdString(), config.url.toStdString());
    loadMetaFromHold();
    loadPendingFromHold();
    loadCacheFromHold();
    m_engine->connectToEngine(config);
}

void DataSync::disconnectFromEngine()
{
    SPDLOG_INFO("DataSync disconnecting from engine");
    m_engine->disconnectFromEngine();
}

bool DataSync::isConnected() const
{
    return m_engine->isConnected();
}

EngineType DataSync::engineType() const
{
    return m_engine->engineType();
}

bool DataSync::isWritable() const
{
    return m_engine->isWritable();
}

// ── 牌组/模型发现 ──

void DataSync::getDeckNames()
{
    m_engine->fetchDeckNames();
}

void DataSync::getModelNames()
{
    m_engine->fetchModelNames();
}

void DataSync::getModelFieldNames(const QString &modelName)
{
    m_engine->fetchModelFieldNames(modelName);
}

// ── 条目查询（从内存缓存同步读取） ──

QList<MemEntry> DataSync::getAllEntries() const
{
    return m_entriesByNoteId.values();
}

QList<MemEntry> DataSync::getDueEntries(int limit) const
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QList<MemEntry> due;
    for (const MemEntry &e : m_entriesByNoteId) {
        if (e.queue == CardQueue::New || e.due <= now)
            due.append(e);
    }
    std::sort(due.begin(), due.end(), [](const MemEntry &a, const MemEntry &b) {
        return a.due < b.due;
    });
    if (limit > 0 && due.size() > limit)
        due = due.mid(0, limit);
    return due;
}

MemEntry DataSync::getEntryByNoteId(qint64 noteId) const
{
    return m_entriesByNoteId.value(noteId);
}

MemEntry DataSync::getEntryByCardId(qint64 cardId) const
{
    qint64 noteId = m_noteIdByCardId.value(cardId, 0);
    if (noteId == 0)
        return {};
    return m_entriesByNoteId.value(noteId);
}

QList<MemEntry> DataSync::searchEntries(const QString &keyword) const
{
    QList<MemEntry> result;
    QString lower = keyword.toLower();
    for (const MemEntry &e : m_entriesByNoteId) {
        bool found = false;
        for (auto it = e.fields.begin(); it != e.fields.end() && !found; ++it)
            found = it.value().toLower().contains(lower);
        if (!found) {
            for (const QString &tag : e.tags) {
                if (tag.toLower().contains(lower)) { found = true; break; }
            }
        }
        if (found)
            result.append(e);
    }
    return result;
}

// ── 笔记管理 ──

void DataSync::addNote(const QMap<QString, QString> &fields, const QStringList &tags)
{
    if (!m_engine->isConnected() || !m_engine->isWritable()) {
        reportError(QStringLiteral("addNote"), QStringLiteral("Engine not connected or not writable"));
        return;
    }
    m_engine->createNote(m_config.deckName, m_config.modelName, fields, tags);
}

void DataSync::updateNoteFields(qint64 noteId, const QMap<QString, QString> &fields)
{
    if (!m_engine->isConnected() || !m_engine->isWritable()) {
        reportError(QStringLiteral("updateNoteFields"), QStringLiteral("Engine not connected or not writable"));
        return;
    }
    m_engine->updateNoteFields(noteId, fields);
}

void DataSync::deleteNote(qint64 noteId)
{
    if (!m_engine->isConnected() || !m_engine->isWritable()) {
        reportError(QStringLiteral("deleteNote"), QStringLiteral("Engine not connected or not writable"));
        return;
    }
    m_engine->deleteNotes({noteId});
}

// ── 复习提交 ──

void DataSync::answerCard(qint64 cardId, EaseRating ease)
{
    answerCards({{cardId, ease}});
}

void DataSync::answerCards(const QList<AnswerRecord> &answers)
{
    if (m_engine->isConnected() && m_engine->isWritable()) {
        // 在线：直接提交到引擎
        m_engine->submitReviews(answers);
    } else {
        // 离线：暂存到本地队列
        m_pendingAnswers.append(answers);
        savePendingToHold();
    }
}

// ── 同步 ──

void DataSync::pullFromEngine()
{
    if (m_syncCtx && m_syncCtx->step != SyncStep::Idle) {
        SPDLOG_DEBUG("DataSync pullFromEngine skipped: sync already in progress");
        return; // 已有同步在进行中
    }

    if (!m_engine->isConnected()) {
        reportError(QStringLiteral("pull"), QStringLiteral("Engine not connected"));
        return;
    }

    SPDLOG_INFO("DataSync pullFromEngine starting for deck={}", m_config.deckName.toStdString());
    m_syncCtx = std::make_shared<SyncContext>();
    m_syncCtx->report.syncAt = QDateTime::currentMSecsSinceEpoch();
    m_syncCtx->step = SyncStep::WaitingForNoteIds;

    QString query = QStringLiteral("deck:") + m_config.deckName;
    m_engine->fetchNoteIds(query);
}

void DataSync::pushToEngine()
{
    if (!m_engine->isConnected() || !m_engine->isWritable()) {
        reportError(QStringLiteral("push"), QStringLiteral("Engine not connected or not writable"));
        return;
    }

    if (m_pendingAnswers.isEmpty()) {
        SyncReport r;
        r.syncAt = QDateTime::currentMSecsSinceEpoch();
        m_lastReport = r;
        emit syncFinished(r);
        return;
    }

    int pendingCount = m_pendingAnswers.size();
    SPDLOG_INFO("DataSync pushToEngine: {} pending answers", pendingCount);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_engine, &ISRSEngine::reviewsSubmitted, this,
        [this, pendingCount, conn](int submitted) {
            Q_UNUSED(submitted);
            disconnect(*conn);
            // 保留 submit 之后新追加的记录
            m_pendingAnswers = m_pendingAnswers.mid(pendingCount);
            savePendingToHold();
            SyncReport r;
            r.answersSubmitted = pendingCount;
            r.syncAt = QDateTime::currentMSecsSinceEpoch();
            m_lastReport = r;
            emit syncFinished(r);
        });

    m_engine->submitReviews(m_pendingAnswers.mid(0, pendingCount));
}

void DataSync::fullSync()
{
    if (!m_engine->isConnected()) {
        reportError(QStringLiteral("fullSync"), QStringLiteral("Engine not connected"));
        return;
    }

    SPDLOG_INFO("DataSync fullSync starting ({} pending answers)", m_pendingAnswers.size());
    if (m_pendingAnswers.isEmpty()) {
        pullFromEngine();
        return;
    }

    // 先 push 再 pull
    int pendingCount = m_pendingAnswers.size();
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_engine, &ISRSEngine::reviewsSubmitted, this,
        [this, pendingCount, conn](int submitted) {
            Q_UNUSED(submitted);
            disconnect(*conn);
            m_pendingAnswers = m_pendingAnswers.mid(pendingCount);
            savePendingToHold();
            pullFromEngine();
        });

    m_engine->submitReviews(m_pendingAnswers.mid(0, pendingCount));
}

SyncReport DataSync::lastSyncReport() const
{
    return m_lastReport;
}

// ── 同步状态机槽函数 ──

void DataSync::onSyncNoteIdsReady(const QList<qint64> &ids)
{
    if (!m_syncCtx || m_syncCtx->step != SyncStep::WaitingForNoteIds)
        return;

    m_syncCtx->allNoteIds = ids;
    SPDLOG_DEBUG("DataSync sync step: got {} note ids", ids.size());

    if (ids.isEmpty()) {
        // 牌组为空，直接结束
        SPDLOG_INFO("DataSync sync finished: deck is empty");
        m_syncCtx->report.syncAt = QDateTime::currentMSecsSinceEpoch();
        m_lastReport = m_syncCtx->report;
        m_syncCtx.reset();
        emit syncFinished(m_lastReport);
        return;
    }

    m_syncCtx->step = SyncStep::WaitingForCardIds;
    QString query = QStringLiteral("deck:") + m_config.deckName;
    m_engine->fetchCardIds(query);
}

void DataSync::onSyncCardIdsReady(const QList<qint64> &ids)
{
    if (!m_syncCtx || m_syncCtx->step != SyncStep::WaitingForCardIds)
        return;

    m_syncCtx->allCardIds = ids;
    SPDLOG_DEBUG("DataSync sync step: got {} card ids", ids.size());
    m_syncCtx->step = SyncStep::WaitingForNotesInfo;
    m_engine->fetchNotesInfo(m_syncCtx->allNoteIds);
}

void DataSync::onSyncNotesInfoReady(const QList<MemEntry> &entries)
{
    if (!m_syncCtx || m_syncCtx->step != SyncStep::WaitingForNotesInfo)
        return;

    m_syncCtx->notesList = entries;
    SPDLOG_DEBUG("DataSync sync step: got {} notes info", entries.size());

    if (m_syncCtx->allCardIds.isEmpty()) {
        // 无卡片数据，直接用 notes 结果
        m_entriesByNoteId.clear();
        m_noteIdByCardId.clear();
        for (const MemEntry &e : entries) {
            m_entriesByNoteId[e.noteId] = e;
        }
        saveCacheToHold();
        m_syncCtx->report.notesPulled = entries.size();
        m_syncCtx->report.syncAt = QDateTime::currentMSecsSinceEpoch();
        m_lastSyncAt = m_syncCtx->report.syncAt;
        m_lastReport = m_syncCtx->report;
        saveMetaToHold();
        m_syncCtx.reset();
        SPDLOG_INFO("DataSync sync finished: {} notes (no cards)", entries.size());
        emit entriesChanged(m_entriesByNoteId.keys());
        emit syncFinished(m_lastReport);
        return;
    }

    m_syncCtx->step = SyncStep::WaitingForCardsInfo;
    m_engine->fetchCardsInfo(m_syncCtx->allCardIds);
}

void DataSync::onSyncCardsInfoReady(const QList<MemEntry> &entries)
{
    if (!m_syncCtx || m_syncCtx->step != SyncStep::WaitingForCardsInfo)
        return;

    m_syncCtx->cardsList = entries;

    // 合并 Note + Card 数据
    QHash<qint64, MemEntry> merged = mergeNoteAndCardData(
        m_syncCtx->notesList, m_syncCtx->cardsList);

    // 更新内存缓存 + 重建索引
    m_entriesByNoteId = merged;
    m_noteIdByCardId.clear();
    for (const MemEntry &e : merged) {
        for (qint64 cid : e.cardIds)
            m_noteIdByCardId[cid] = e.noteId;
    }

    saveCacheToHold();

    m_syncCtx->report.notesPulled = merged.size();
    m_syncCtx->report.syncAt = QDateTime::currentMSecsSinceEpoch();
    m_lastSyncAt = m_syncCtx->report.syncAt;
    m_lastReport = m_syncCtx->report;
    saveMetaToHold();

    QList<qint64> changedIds = m_entriesByNoteId.keys();
    m_syncCtx.reset();

    SPDLOG_INFO("DataSync sync finished: {} merged entries", merged.size());
    emit entriesChanged(changedIds);
    emit syncFinished(m_lastReport);
}

void DataSync::finishSyncWithError(const QString &error)
{
    SPDLOG_ERROR("DataSync sync failed: {}", error.toStdString());
    if (m_syncCtx) {
        m_syncCtx->report.errors.append(error);
        m_syncCtx->report.syncAt = QDateTime::currentMSecsSinceEpoch();
        m_lastReport = m_syncCtx->report;
        m_syncCtx.reset();
    }
    reportError(QStringLiteral("sync"), error);
    emit syncFinished(m_lastReport);
}

// ── 缓存管理 ──

static const QStringList kCacheId = {QStringLiteral("datasync"), QStringLiteral("cache")};
static const QStringList kMetaId  = {QStringLiteral("datasync"), QStringLiteral("meta")};
static const QStringList kPendingId = {QStringLiteral("datasync"), QStringLiteral("pending")};

void DataSync::loadCacheFromHold()
{
    QByteArray data = m_hold->load(kCacheId, m_config.deckName);
    if (data.isEmpty())
        return;
    QList<MemEntry> list = deserializeEntries(data);
    m_entriesByNoteId.clear();
    m_noteIdByCardId.clear();
    for (const MemEntry &e : list) {
        m_entriesByNoteId[e.noteId] = e;
        for (qint64 cid : e.cardIds)
            m_noteIdByCardId[cid] = e.noteId;
    }
}

void DataSync::saveCacheToHold()
{
    QList<MemEntry> list = m_entriesByNoteId.values();
    m_hold->save(kCacheId, m_config.deckName, serializeEntries(list));
}

void DataSync::loadMetaFromHold()
{
    QByteArray data = m_hold->load(kMetaId, m_config.deckName);
    if (data.isEmpty())
        return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        m_lastSyncAt = static_cast<qint64>(doc.object().value("lastSyncAt").toDouble());
    }
}

void DataSync::saveMetaToHold()
{
    QJsonObject obj;
    obj["lastSyncAt"] = m_lastSyncAt;
    m_hold->save(kMetaId, m_config.deckName,
                 QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void DataSync::loadPendingFromHold()
{
    QByteArray data = m_hold->load(kPendingId, m_config.deckName);
    if (data.isEmpty())
        return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return;
    m_pendingAnswers.clear();
    for (const QJsonValue &v : doc.array()) {
        QJsonObject obj = v.toObject();
        AnswerRecord ar;
        ar.cardId = static_cast<qint64>(obj["cardId"].toDouble());
        ar.ease = static_cast<EaseRating>(obj["ease"].toInt());
        m_pendingAnswers.append(ar);
    }
}

void DataSync::savePendingToHold()
{
    QJsonArray arr;
    for (const AnswerRecord &a : m_pendingAnswers) {
        QJsonObject obj;
        obj["cardId"] = a.cardId;
        obj["ease"] = static_cast<int>(a.ease);
        arr.append(obj);
    }
    m_hold->save(kPendingId, m_config.deckName,
                 QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// ── 数据合并与序列化 ──

QHash<qint64, MemEntry> DataSync::mergeNoteAndCardData( /* non-static: uses m_config */
    const QList<MemEntry> &notes, const QList<MemEntry> &cards)
{
    // 构建 cardId → noteId 索引 (notesInfo 不包含 cardId, cardsInfo 包含 note 字段)
    // cardsInfo 已通过 parseCardInfo 设置了 noteId
    QHash<qint64, QList<MemEntry>> cardsByNoteId;
    for (const MemEntry &card : cards)
        cardsByNoteId[card.noteId].append(card);

    QHash<qint64, MemEntry> result;
    for (const MemEntry &note : notes) {
        MemEntry entry = note;
        entry.source = EngineType::AnkiConnect;
        entry.deckName = m_config.deckName;

        auto it = cardsByNoteId.find(note.noteId);
        if (it != cardsByNoteId.end() && !it->isEmpty()) {
            const MemEntry &primary = it->first();
            entry.cardId = primary.cardId;
            for (const MemEntry &c : *it)
                entry.cardIds.append(c.cardId);
            entry.deckName = primary.deckName.isEmpty() ? entry.deckName : primary.deckName;
            entry.easeFactor = primary.easeFactor;
            entry.intervalDays = primary.intervalDays;
            entry.due = primary.due;
            entry.reps = primary.reps;
            entry.lapses = primary.lapses;
            entry.queue = primary.queue;
            entry.lastReviewedAt = primary.lastReviewedAt;
        }
        result[entry.noteId] = entry;
    }
    return result;
}

QByteArray DataSync::serializeEntries(const QList<MemEntry> &entries)
{
    QJsonArray arr;
    for (const MemEntry &e : entries) {
        QJsonObject obj;
        obj["noteId"] = e.noteId;
        obj["cardId"] = e.cardId;
        QJsonArray cids;
        for (qint64 c : e.cardIds) cids.append(c);
        obj["cardIds"] = cids;
        obj["deckName"] = e.deckName;
        obj["modelName"] = e.modelName;
        QJsonObject f;
        for (auto it = e.fields.begin(); it != e.fields.end(); ++it)
            f[it.key()] = it.value();
        obj["fields"] = f;
        QJsonArray t;
        for (const QString &tag : e.tags) t.append(tag);
        obj["tags"] = t;
        obj["source"] = static_cast<int>(e.source);
        obj["easeFactor"] = e.easeFactor;
        obj["intervalDays"] = e.intervalDays;
        obj["due"] = e.due;
        obj["reps"] = e.reps;
        obj["lapses"] = e.lapses;
        obj["queue"] = static_cast<int>(e.queue);
        obj["lastReviewedAt"] = e.lastReviewedAt;
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

QList<MemEntry> DataSync::deserializeEntries(const QByteArray &data)
{
    QList<MemEntry> result;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return result;
    for (const QJsonValue &v : doc.array()) {
        QJsonObject obj = v.toObject();
        MemEntry e;
        e.noteId = static_cast<qint64>(obj["noteId"].toDouble());
        e.cardId = static_cast<qint64>(obj["cardId"].toDouble());
        for (const QJsonValue &c : obj["cardIds"].toArray())
            e.cardIds.append(static_cast<qint64>(c.toDouble()));
        e.deckName = obj["deckName"].toString();
        e.modelName = obj["modelName"].toString();
        QJsonObject f = obj["fields"].toObject();
        for (auto it = f.begin(); it != f.end(); ++it)
            e.fields[it.key()] = it.value().toString();
        for (const QJsonValue &t : obj["tags"].toArray())
            e.tags.append(t.toString());
        e.source = static_cast<EngineType>(obj["source"].toInt());
        e.easeFactor = static_cast<float>(obj["easeFactor"].toDouble());
        e.intervalDays = static_cast<float>(obj["intervalDays"].toDouble());
        e.due = static_cast<qint64>(obj["due"].toDouble());
        e.reps = obj["reps"].toInt();
        e.lapses = obj["lapses"].toInt();
        e.queue = static_cast<CardQueue>(obj["queue"].toInt());
        e.lastReviewedAt = static_cast<qint64>(obj["lastReviewedAt"].toDouble());
        result.append(e);
    }
    return result;
}

// ── 错误报告 ──

void DataSync::reportError(const QString &code, const QString &message)
{
    SPDLOG_WARN("DataSync error [{}]: {}", code.toStdString(), message.toStdString());
    emit errorOccurred({code, message});
}
