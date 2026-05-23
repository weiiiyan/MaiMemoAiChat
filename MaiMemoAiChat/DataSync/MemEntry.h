#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QMetaType>

/// SRS 引擎类型
enum class EngineType {
    AnkiConnect,
    MoMoBei
};

/// Anki 卡片队列类型 (queue 字段值)
enum class CardQueue {
    New = 0,
    Learn = 1,
    Review = 2,
    Due = 3
};

/// Anki 评分等级 (answerCards API 的 ease 参数)
enum class EaseRating {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4
};

/**
 * @brief 统一记忆条目 — 对应一个 Anki Note + 其主 Card
 *
 * SRS 参数从 Anki Card 同步到本地缓存，用于离线显示和查询。
 * getDueEntries 按 Card 的 due 时间决定是否到期——任意一张 Card 到期即返回该条目。
 */
struct MemEntry {
    qint64 noteId = 0;
    qint64 cardId = 0;            ///< 主卡片 ID（取首张 Card）
    QList<qint64> cardIds;        ///< 关联的全部卡片 ID
    QString deckName;
    QString modelName;
    QMap<QString, QString> fields; ///< 原始字段（映射到 Anki Note fields）
    QStringList tags;
    EngineType source = EngineType::AnkiConnect;

    // SRS 参数（从 Anki Card 同步）
    float easeFactor = 0.0f;
    float intervalDays = 0.0f;
    qint64 due = 0;              ///< 到期时间戳
    int reps = 0;                ///< 总复习次数
    int lapses = 0;              ///< 遗忘次数
    CardQueue queue = CardQueue::New;
    qint64 lastReviewedAt = 0;   ///< 最后复习时间戳
};

/// 复习作答记录
struct AnswerRecord {
    qint64 cardId = 0;
    EaseRating ease = EaseRating::Good;
};

/// SRS 引擎连接配置
struct SRSConfig {
    QString url = QStringLiteral("http://localhost:8765");
    QString deckName;
    QString modelName;
    QMap<QString, QString> fieldMapping; ///< app 逻辑字段 → Anki 字段名
    int apiVersion = 6;
};

/// 同步结果报告
struct SyncReport {
    int notesPulled = 0;
    int notesPushed = 0;
    int answersSubmitted = 0;
    qint64 syncAt = 0;
    QStringList errors;
};

/// 同步/引擎错误
struct SyncError {
    QString code;
    QString message;
};

Q_DECLARE_METATYPE(MemEntry)
Q_DECLARE_METATYPE(AnswerRecord)
Q_DECLARE_METATYPE(SRSConfig)
Q_DECLARE_METATYPE(SyncReport)
Q_DECLARE_METATYPE(SyncError)
