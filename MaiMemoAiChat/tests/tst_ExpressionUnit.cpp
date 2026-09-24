#include "domain/ExpressionUnit.h"

#include <QTest>

#include <optional>

using mai::domain::ExpressionUnit;

class TestExpressionUnit : public QObject
{
    Q_OBJECT

private slots:
    void create_rejectsEmptyFormOrSense();
    void create_rejectsWhitespaceOnlyFields();
    void create_keepsFormAndSenseVerbatim();
    void normalizedForm_ignoresCaseAndSurroundingWhitespace();
    void normalizedSense_trimsButKeepsCase();
    void sameIdentityAs_matchesOnNormalizedParts();
    void sameIdentityAs_distinguishesSensesOfTheSameForm();
    void sameIdentityAs_keepsInnerWhitespaceSignificant();
};

void TestExpressionUnit::create_rejectsEmptyFormOrSense()
{
    QVERIFY(!ExpressionUnit::create(QString(), QStringLiteral("放弃")).has_value());
    QVERIFY(!ExpressionUnit::create(QStringLiteral("give up"), QString()).has_value());
}

void TestExpressionUnit::create_rejectsWhitespaceOnlyFields()
{
    QVERIFY(!ExpressionUnit::create(QStringLiteral("   "), QStringLiteral("放弃")).has_value());
    QVERIFY(!ExpressionUnit::create(QStringLiteral("give up"), QStringLiteral(" \t ")).has_value());
}

void TestExpressionUnit::create_keepsFormAndSenseVerbatim()
{
    const std::optional<ExpressionUnit> unit =
            ExpressionUnit::create(QStringLiteral("  Give Up  "), QStringLiteral(" 放弃 "));

    QVERIFY(unit.has_value());
    QCOMPARE(unit->form(), QStringLiteral("  Give Up  "));
    QCOMPARE(unit->sense(), QStringLiteral(" 放弃 "));
}

void TestExpressionUnit::normalizedForm_ignoresCaseAndSurroundingWhitespace()
{
    const std::optional<ExpressionUnit> unit =
            ExpressionUnit::create(QStringLiteral("  Give Up  "), QStringLiteral("放弃"));

    QVERIFY(unit.has_value());
    QCOMPARE(unit->normalizedForm(), QStringLiteral("give up"));
}

void TestExpressionUnit::normalizedSense_trimsButKeepsCase()
{
    const std::optional<ExpressionUnit> unit =
            ExpressionUnit::create(QStringLiteral("apple"), QStringLiteral("  Apple 公司 "));

    QVERIFY(unit.has_value());
    QCOMPARE(unit->normalizedSense(), QStringLiteral("Apple 公司"));
}

void TestExpressionUnit::sameIdentityAs_matchesOnNormalizedParts()
{
    const std::optional<ExpressionUnit> a =
            ExpressionUnit::create(QStringLiteral("give up"), QStringLiteral("放弃"));
    const std::optional<ExpressionUnit> b =
            ExpressionUnit::create(QStringLiteral("  GIVE UP "), QStringLiteral(" 放弃 "));

    QVERIFY(a.has_value());
    QVERIFY(b.has_value());
    QVERIFY(a->sameIdentityAs(*b));
}

void TestExpressionUnit::sameIdentityAs_distinguishesSensesOfTheSameForm()
{
    const std::optional<ExpressionUnit> givingUp =
            ExpressionUnit::create(QStringLiteral("give up"), QStringLiteral("放弃"));
    const std::optional<ExpressionUnit> surrendering =
            ExpressionUnit::create(QStringLiteral("give up"), QStringLiteral("投降"));

    QVERIFY(givingUp.has_value());
    QVERIFY(surrendering.has_value());
    QVERIFY(!givingUp->sameIdentityAs(*surrendering));
}

void TestExpressionUnit::sameIdentityAs_keepsInnerWhitespaceSignificant()
{
    const std::optional<ExpressionUnit> single =
            ExpressionUnit::create(QStringLiteral("give up"), QStringLiteral("放弃"));
    const std::optional<ExpressionUnit> doubleSpaced =
            ExpressionUnit::create(QStringLiteral("give  up"), QStringLiteral("放弃"));

    QVERIFY(single.has_value());
    QVERIFY(doubleSpaced.has_value());
    QVERIFY(!single->sameIdentityAs(*doubleSpaced));
}

QTEST_MAIN(TestExpressionUnit)
#include "tst_ExpressionUnit.moc"
