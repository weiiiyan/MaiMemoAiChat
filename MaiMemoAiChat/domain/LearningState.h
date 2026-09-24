#pragma once

#include <QString>

#include <optional>

namespace mai::domain {

/// 维度：决定进入哪种练习；理解内嵌于练习（R3）
enum class Dimension { Reading, Listening, Speaking, Writing };

/// 学习状态：三态互斥，同一时刻只一种（R4）
enum class LearningState { NotLearned, InReview, Mastered };

// 词汇表——由领域层定义，适配层与框架层不得自造字符串
QString toString(Dimension dimension);
std::optional<Dimension> parseDimension(const QString& text);

QString toString(LearningState state);
std::optional<LearningState> parseLearningState(const QString& text);

} // namespace mai::domain
