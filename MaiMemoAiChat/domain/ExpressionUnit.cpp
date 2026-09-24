#include "domain/ExpressionUnit.h"

#include <utility>

namespace mai::domain {

ExpressionUnitKey::ExpressionUnitKey(QString normalizedForm, QString normalizedSense)
    : m_normalizedForm(std::move(normalizedForm))
    , m_normalizedSense(std::move(normalizedSense))
{
}

bool ExpressionUnitKey::operator==(const ExpressionUnitKey& other) const
{
    return m_normalizedForm == other.m_normalizedForm
            && m_normalizedSense == other.m_normalizedSense;
}

bool ExpressionUnitKey::operator<(const ExpressionUnitKey& other) const
{
    // 词形在前、义项在后：有序容器里的顺序因此是确定的
    if (m_normalizedForm != other.m_normalizedForm)
        return m_normalizedForm < other.m_normalizedForm;
    return m_normalizedSense < other.m_normalizedSense;
}

std::optional<ExpressionUnit> ExpressionUnit::create(const QString& form, const QString& sense)
{
    // 必填：纯空白也算空
    if (form.trimmed().isEmpty() || sense.trimmed().isEmpty())
        return std::nullopt;

    return ExpressionUnit(form, sense);
}

ExpressionUnit::ExpressionUnit(QString form, QString sense)
    : m_form(std::move(form))
    , m_sense(std::move(sense))
{
}

const QString& ExpressionUnit::form() const
{
    return m_form;
}

const QString& ExpressionUnit::sense() const
{
    return m_sense;
}

QString ExpressionUnit::normalizedForm() const
{
    // 只去首尾空白、只降大小写：词形中间的空格是有意义的
    return m_form.trimmed().toLower();
}

QString ExpressionUnit::normalizedSense() const
{
    // 只去首尾空白：义项里可能出现专名，大小写不能碰
    return m_sense.trimmed();
}

ExpressionUnitKey ExpressionUnit::identityKey() const
{
    return {normalizedForm(), normalizedSense()};
}

bool ExpressionUnit::sameIdentityAs(const ExpressionUnit& other) const
{
    return identityKey() == other.identityKey();
}

} // namespace mai::domain
