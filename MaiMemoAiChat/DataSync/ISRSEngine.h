#pragma once

#include <QObject>
#include "MemEntry.h"

/**
 * @brief SRS 引擎适配层抽象接口
 *
 * 屏蔽不同 SRS 引擎（AnkiConnect / 墨墨背单词）的差异，
 * 提供统一的笔记查询、复习提交和同步方法。
 * 所有方法均为异步——调用后通过对应信号返回结果。
 */
class ISRSEngine : public QObject
{
    Q_OBJECT

public:
    explicit ISRSEngine(QObject *parent = nullptr) : QObject(parent) {}
    ~ISRSEngine() override = default;

    /// 连接引擎（验证可用性）
    virtual void connectToEngine(const SRSConfig &config) = 0;
    /// 断开引擎连接
    virtual void disconnectFromEngine() = 0;
    /// 引擎是否已连接
    virtual bool isConnected() const = 0;
    /// 返回引擎类型
    virtual EngineType engineType() const = 0;
    /// 引擎是否支持写入（墨墨背单词等可能不支持）
    virtual bool isWritable() const = 0;

    // ── 牌组/模型发现 ──

    virtual void fetchDeckNames() = 0;
    virtual void fetchModelNames() = 0;
    virtual void fetchModelFieldNames(const QString &modelName) = 0;

    // ── 数据抓取 ──

    /// @param query Anki 搜索语法，如 "deck:English"
    virtual void fetchNoteIds(const QString &query) = 0;
    /// @param query Anki 搜索语法，返回匹配的卡片 ID（findCards API）
    virtual void fetchCardIds(const QString &query) = 0;
    virtual void fetchNotesInfo(const QList<qint64> &noteIds) = 0;
    virtual void fetchCardsInfo(const QList<qint64> &cardIds) = 0;
    /// @param since 时间戳（毫秒），拉取此后修改的 noteId
    virtual void fetchModifiedNoteIds(qint64 since) = 0;

    // ── 笔记 CRUD ──

    virtual void createNote(const QString &deckName, const QString &modelName,
                            const QMap<QString, QString> &fields,
                            const QStringList &tags) = 0;
    virtual void updateNoteFields(qint64 noteId,
                                  const QMap<QString, QString> &fields) = 0;
    virtual void deleteNotes(const QList<qint64> &noteIds) = 0;

    // ── 复习提交 ──

    virtual void submitReviews(const QList<AnswerRecord> &answers) = 0;

signals:
    void connected();
    void disconnected(const QString &reason);

    void deckNamesReady(const QStringList &names);
    void modelNamesReady(const QStringList &names);
    void modelFieldNamesReady(const QStringList &fields);

    void noteIdsReady(const QList<qint64> &ids);
    void cardIdsReady(const QList<qint64> &ids);
    void notesInfoReady(const QList<MemEntry> &entries);
    void cardsInfoReady(const QList<MemEntry> &entries);
    void modifiedNoteIdsReady(const QList<qint64> &ids);

    void noteCreated(qint64 noteId);
    void noteFieldsUpdated(qint64 noteId);
    void notesDeleted(const QList<qint64> &ids);
    void reviewsSubmitted(int count);

    void engineError(const QString &error);
};
