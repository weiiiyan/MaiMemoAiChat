#include "domain/LearningState.h"

namespace mai::domain {

QString toString(Dimension dimension)
{
    switch (dimension) {
    case Dimension::Reading:
        return QStringLiteral("reading");
    case Dimension::Listening:
        return QStringLiteral("listening");
    case Dimension::Speaking:
        return QStringLiteral("speaking");
    case Dimension::Writing:
        return QStringLiteral("writing");
    }
    Q_UNREACHABLE_RETURN(QString());
}

std::optional<Dimension> parseDimension(const QString& text)
{
    if (text == QStringLiteral("reading"))
        return Dimension::Reading;
    if (text == QStringLiteral("listening"))
        return Dimension::Listening;
    if (text == QStringLiteral("speaking"))
        return Dimension::Speaking;
    if (text == QStringLiteral("writing"))
        return Dimension::Writing;
    return std::nullopt;
}

QString toString(LearningState state)
{
    switch (state) {
    case LearningState::NotLearned:
        return QStringLiteral("notLearned");
    case LearningState::InReview:
        return QStringLiteral("inReview");
    case LearningState::Mastered:
        return QStringLiteral("mastered");
    }
    Q_UNREACHABLE_RETURN(QString());
}

std::optional<LearningState> parseLearningState(const QString& text)
{
    if (text == QStringLiteral("notLearned"))
        return LearningState::NotLearned;
    if (text == QStringLiteral("inReview"))
        return LearningState::InReview;
    if (text == QStringLiteral("mastered"))
        return LearningState::Mastered;
    return std::nullopt;
}

} // namespace mai::domain
