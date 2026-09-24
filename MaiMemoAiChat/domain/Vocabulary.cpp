#include "domain/Vocabulary.h"

namespace mai::domain {

Vocabulary::InsertResult Vocabulary::insert(const ExpressionUnit& entry)
{
    const ExpressionUnitKey key = entry.identityKey();
    if (m_entries.contains(key))
        return InsertResult::Duplicate;

    m_entries.insert(key, entry);
    return InsertResult::Added;
}

bool Vocabulary::contains(const ExpressionUnit& entry) const
{
    return m_entries.contains(entry.identityKey());
}

std::optional<ExpressionUnit> Vocabulary::find(const ExpressionUnit& entry) const
{
    const auto it = m_entries.constFind(entry.identityKey());
    if (it == m_entries.constEnd())
        return std::nullopt;
    return *it;
}

QList<ExpressionUnit> Vocabulary::all() const
{
    QList<ExpressionUnit> entries;
    entries.reserve(m_entries.size());
    for (const ExpressionUnit& entry : m_entries)
        entries.append(entry);
    return entries;
}

} // namespace mai::domain
