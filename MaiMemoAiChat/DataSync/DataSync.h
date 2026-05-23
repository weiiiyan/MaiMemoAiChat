#pragma once

#include "IDataSync.h"
#include <QHash>
#include <memory>

class Hold;
class ISRSEngine;

/**
 * @brief IDataSync 的具体实现，组合 ISRSEngine + Hold 本地缓存
 *
 * 架构：
 *   IDataSync ──> ISRSEngine ──> AnkiConnectEngine  (HTTP)
 *            └──> Hold                              (本地缓存)
 *
 * 同步流程（pullFromEngine）：
 *   1. fetchNoteIds("deck:XXX")  →  获取牌组内全部 Note ID
 *   2. fetchCardIds("deck:XXX")  →  获取牌组内全部 Card ID
 *   3. fetchNotesInfo(noteIds)   →  拉取 Note 字段/标签/模型
 *   4. fetchCardsInfo(cardIds)   →  拉取 Card SRS 参数
 *   5. 合并 Note+Card → 更新本地缓存 → emit syncFinished
 *
 * 复习提交离线队列：
 *   answerCard 若引擎不可用则暂存到 pendingAnswers，下次 pushToEngine 时回写。
 */
class DataSync : public IDataSync
{
    Q_OBJECT

public:
    explicit DataSync(ISRSEngine *engine, Hold *hold, QObject *parent = nullptr);
    ~DataSync() override = default;

    // ── 引擎生命周期 ──
    void connectToEngine(const SRSConfig &config) override;
    void disconnectFromEngine() override;
    bool isConnected() const override;
    EngineType engineType() const override;
    bool isWritable() const override;

    // ── 牌组/模型发现 ──
    void getDeckNames() override;
    void getModelNames() override;
    void getModelFieldNames(const QString &modelName) override;

    // ── 条目查询 ──
    QList<MemEntry> getAllEntries() const override;
    QList<MemEntry> getDueEntries(int limit = -1) const override;
    MemEntry getEntryByNoteId(qint64 noteId) const override;
    MemEntry getEntryByCardId(qint64 cardId) const override;
    QList<MemEntry> searchEntries(const QString &keyword) const override;

    // ── 笔记管理 ──
    void addNote(const QMap<QString, QString> &fields,
                 const QStringList &tags = {}) override;
    void updateNoteFields(qint64 noteId,
                          const QMap<QString, QString> &fields) override;
    void deleteNote(qint64 noteId) override;

    // ── 复习提交 ──
    void answerCard(qint64 cardId, EaseRating ease) override;
    void answerCards(const QList<AnswerRecord> &answers) override;

    // ── 同步 ──
    void pullFromEngine() override;
    void pushToEngine() override;
    void fullSync() override;
    SyncReport lastSyncReport() const override;

private:
    // ── 同步状态机 ──
    enum class SyncStep {
        Idle,
        WaitingForNoteIds,
        WaitingForCardIds,
        WaitingForNotesInfo,
        WaitingForCardsInfo
    };

    struct SyncContext {
        SyncStep step = SyncStep::Idle;
        SyncReport report;
        QList<qint64> allNoteIds;
        QList<qint64> allCardIds;
        QList<MemEntry> notesList;
        QList<MemEntry> cardsList;
    };

    void onSyncNoteIdsReady(const QList<qint64> &ids);
    void onSyncCardIdsReady(const QList<qint64> &ids);
    void onSyncNotesInfoReady(const QList<MemEntry> &entries);
    void onSyncCardsInfoReady(const QList<MemEntry> &entries);
    void finishSyncWithError(const QString &error);

    // ── 缓存管理 ──
    void loadCacheFromHold();
    void saveCacheToHold();
    void loadMetaFromHold();
    void saveMetaToHold();
    void loadPendingFromHold();
    void savePendingToHold();

    /// 将 notesInfo + cardsInfo 结果合并为完整 MemEntry，以 noteId 为 key
    QHash<qint64, MemEntry> mergeNoteAndCardData(
        const QList<MemEntry> &notes,
        const QList<MemEntry> &cards);
    static QByteArray serializeEntries(const QList<MemEntry> &entries);
    static QList<MemEntry> deserializeEntries(const QByteArray &data);

    /// 创建 SyncError 并 emit errorOccurred
    void reportError(const QString &code, const QString &message);

    // ── 成员 ──
    ISRSEngine *m_engine;
    Hold *m_hold;
    SRSConfig m_config;
    SyncReport m_lastReport;
    qint64 m_lastSyncAt = 0;

    /// 内存缓存：noteId → MemEntry（查询时从此读取）
    QHash<qint64, MemEntry> m_entriesByNoteId;
    /// 反向索引：cardId → noteId
    QHash<qint64, qint64> m_noteIdByCardId;

    /// 待推送的复习记录
    QList<AnswerRecord> m_pendingAnswers;

    /// 当前同步上下文（仅允许一次一个同步操作）
    std::shared_ptr<SyncContext> m_syncCtx;
};
