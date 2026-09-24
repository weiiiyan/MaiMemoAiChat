#pragma once

#include "domain/ExpressionUnit.h"

#include <QTest>

namespace mai::test {

/// 测试里只喂合法输入；造不出来说明测试自己写错了
inline mai::domain::ExpressionUnit makeUnit(const QString& form, const QString& sense)
{
    const std::optional<mai::domain::ExpressionUnit> unit =
            mai::domain::ExpressionUnit::create(form, sense);
    Q_ASSERT(unit.has_value());
    return *unit;
}

} // namespace mai::test
