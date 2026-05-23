#pragma once

#include <QObject>
#include "MemEntry.h"

/**
 * @brief 数据同步模块主接口 — AppCoordinator 通过此接口与 SRS 引擎交互
 *
 * 职责：
 * - 连接管理（connect / disconnect）
 * - 牌组/模型发现
 * - 条目查询（从本地 Hold 缓存读取，不阻塞主线程等待 HTTP）
 * - 笔记管理（CRUD → 回写 Anki）
 * - 复习提交（answerCard → 转发到 Anki 或暂存本地队列）
 * - 双向同步（pullFromEngine / pushToEngine / fullSync）
 *
 * 所有需要网络的操作均为异步，通过信号返回结果。
 */
class IDataSync : public QObject
{
    Q_OBJECT

public:
    explicit IDataSync(QObject *parent = nullptr) : QObject(parent) {}
    ~IDataSync() override = default;

    // ── 引擎生命周期 ──

    virtual void connectToEngine(const SRSConfig &config) = 0;
    virtual void disconnectFromEngine() = 0;
    virtual bool isConnected() const = 0;
    virtual EngineType engineType() const = 0;
    virtual bool isWritable() const = 0;

    // ── 牌组/模型发现（异步，通过信号返回） ──

    virtual void getDeckNames() = 0;
    virtual void getModelNames() = 0;
    virtual void getModelFieldNames(const QString &modelName) = 0;

    // ── 条目查询（同步，从本地缓存读取） ──

    /// 获取缓存中的全部条目
    virtual QList<MemEntry> getAllEntries() const = 0;
    /// 获取到期条目（任一 Card due ≤ now），按 due 升序
    virtual QList<MemEntry> getDueEntries(int limit = -1) const = 0;
    /// 按 noteId 查找单条（缓存未命中返回空 MemEntry，noteId==0）
    virtual MemEntry getEntryByNoteId(qint64 noteId) const = 0;
    /// 按 cardId 查找单条
    virtual MemEntry getEntryByCardId(qint64 cardId) const = 0;
    /// 在缓存的 fields 和 tags 中搜索关键词（大小写不敏感）
    virtual QList<MemEntry> searchEntries(const QString &keyword) const = 0;

    // ── 笔记管理（异步，写入 SRS 引擎） ──

    virtual void addNote(const QMap<QString, QString> &fields,
                         const QStringList &tags = {}) = 0;
    virtual void updateNoteFields(qint64 noteId,
                                  const QMap<QString, QString> &fields) = 0;
    virtual void deleteNote(qint64 noteId) = 0;

    // ── 复习提交 ──

    /// 提交单张卡片的复习评分；若引擎不可用则暂存本地队列
    virtual void answerCard(qint64 cardId, EaseRating ease) = 0;
    /// 批量提交
    virtual void answerCards(const QList<AnswerRecord> &answers) = 0;

    // ── 双向同步（异步，通过 syncFinished 返回结果） ──

    /// 从引擎拉取增量变更，更新本地缓存
    virtual void pullFromEngine() = 0;
    /// 将本地暂存的复习记录推送至引擎
    virtual void pushToEngine() = 0;
    /// 先 push 再 pull
    virtual void fullSync() = 0;
    /// 获取最近一次同步报告
    virtual SyncReport lastSyncReport() const = 0;

signals:
    void connected();
    void disconnected(const QString &reason);

    void deckNamesReady(const QStringList &names);
    void modelNamesReady(const QStringList &names);
    void modelFieldNamesReady(const QStringList &fields);

    /// 本地缓存发生变更（同步完成或手动增删改后），参数为受影响的 noteId 列表
    void entriesChanged(const QList<qint64> &updatedNoteIds);

    void syncFinished(const SyncReport &report);

    void noteAdded(qint64 noteId);
    void noteFieldsUpdated(qint64 noteId);
    void noteDeleted(qint64 noteId);

    void errorOccurred(const SyncError &error);
};
