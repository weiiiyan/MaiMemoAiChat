#pragma once

#include <QString>
#include <QList>
#include <QMetaType>
#include "../DataSync/MemEntry.h"

/// 学习场景类型 — 决定 AI 扮演的角色
enum class SceneType {
    Reading,   ///< 阅读引导者
    Writing,   ///< 写作教练
    Listening, ///< 听力陪练
    Speaking   ///< 口语教练
};

/// 会话状态
enum class SessionStatus {
    Creating,  ///< 正在创建中（等待 AI 开场白）
    Active,    ///< 对话进行中
    Ended      ///< 已结束
};

/// AI 服务商
enum class AiProvider {
    WenXin,    ///< 百度文心一言
    QianWen    ///< 阿里通义千问
};

/// 场景创建配置
struct SceneConfig {
    AiProvider aiProvider = AiProvider::QianWen;
    QString model;           ///< 模型名称，如 "ernie-4.0-turbo-128k" / "qwen-max"
    QString apiKey;          ///< API Key（千问）或 Client ID（文心）
    QString secretKey;       ///< Secret Key（仅文心需要，用于获取 access_token）
    float temperature = 0.7f;
    int maxTokens = 2048;
    int topP = 0;            ///< 0 表示不使用
};

/// 一条对话消息
struct ChatMessage {
    QString role;            ///< "system" / "user" / "assistant"
    QString content;
};

/// 学习对话会话
struct SceneSession {
    QString id;
    SceneType type = SceneType::Reading;
    QList<MemEntry> entries;
    QString systemPrompt;             ///< 创建时根据 type + entries 生成的系统提示词
    int roundCount = 0;
    SessionStatus status = SessionStatus::Creating;
    qint64 createdAt = 0;
    QList<ChatMessage> history;       ///< 对话历史
};

Q_DECLARE_METATYPE(SceneType)
Q_DECLARE_METATYPE(SessionStatus)
Q_DECLARE_METATYPE(AiProvider)
Q_DECLARE_METATYPE(SceneSession)
Q_DECLARE_METATYPE(ChatMessage)
