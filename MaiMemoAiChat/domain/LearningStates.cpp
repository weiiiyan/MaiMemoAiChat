#include "domain/LearningStates.h"

namespace mai::domain {

LearningState LearningStates::stateOf(const ExpressionUnit& entry, Dimension dimension) const
{
    const auto bucket = m_byDimension.constFind(dimension);
    if (bucket == m_byDimension.constEnd())
        return LearningState::NotLearned;

    const auto record = bucket->constFind(entry.identityKey());
    if (record == bucket->constEnd())
        return LearningState::NotLearned;

    return record->state;
}

void LearningStates::setState(const ExpressionUnit& entry, Dimension dimension, LearningState state)
{
    if (state == LearningState::NotLearned) {
        // 未学习是缺省态，落回缺省即删记录
        const auto bucket = m_byDimension.find(dimension);
        if (bucket != m_byDimension.end())
            bucket->remove(entry.identityKey());
        return;
    }

    m_byDimension[dimension].insert(entry.identityKey(), Record{entry, state});
}

QList<ExpressionUnit> LearningStates::masteredIn(Dimension dimension) const
{
    QList<ExpressionUnit> mastered;

    const auto bucket = m_byDimension.constFind(dimension);
    if (bucket == m_byDimension.constEnd())
        return mastered;

    for (const Record& record : *bucket) {
        if (record.state == LearningState::Mastered)
            mastered.append(record.unit);
    }

    return mastered;
}

} // namespace mai::domain
