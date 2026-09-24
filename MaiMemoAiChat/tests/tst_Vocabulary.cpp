#include "domain/Vocabulary.h"
#include "TestUnits.h"

#include <QTest>

#include <optional>

using mai::domain::ExpressionUnit;
using mai::domain::ExpressionUnitKey;
using mai::domain::Vocabulary;
using mai::test::makeUnit;

class TestVocabulary : public QObject
{
    Q_OBJECT

private slots:
    void insert_addsNewEntry();
    void insert_rejectsSameIdentityIgnoringCaseAndSurroundingWhitespace();
    void insert_keepsInnerWhitespaceSignificant();
    void insert_acceptsDifferentSenseOfTheSameForm();
    void containsAndFind_lookUpByIdentity();
    void all_returnsEveryEntryInStableOrder();
};

void TestVocabulary::insert_addsNewEntry()
{
    Vocabulary vocabulary;

    QVERIFY(vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")))
            == Vocabulary::InsertResult::Added);
}

void TestVocabulary::insert_rejectsSameIdentityIgnoringCaseAndSurroundingWhitespace()
{
    Vocabulary vocabulary;
    vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")));

    QVERIFY(vocabulary.insert(makeUnit(QStringLiteral("  GIVE UP  "), QStringLiteral(" 放弃 ")))
            == Vocabulary::InsertResult::Duplicate);
    QCOMPARE(vocabulary.all().size(), 1);

    // 后到者不覆盖先到者：取回的仍是第一次写入的原样词形
    const std::optional<ExpressionUnit> kept =
            vocabulary.find(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")));
    QVERIFY(kept.has_value());
    QCOMPARE(kept->form(), QStringLiteral("give up"));
}

void TestVocabulary::insert_keepsInnerWhitespaceSignificant()
{
    // 只去首尾空白：词形中间的空格是有意义的，两条是不同的表达单元
    Vocabulary vocabulary;
    vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")));

    QVERIFY(vocabulary.insert(makeUnit(QStringLiteral("give  up"), QStringLiteral("放弃")))
            == Vocabulary::InsertResult::Added);
}

void TestVocabulary::insert_acceptsDifferentSenseOfTheSameForm()
{
    Vocabulary vocabulary;
    vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")));

    QVERIFY(vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("投降")))
            == Vocabulary::InsertResult::Added);
}

void TestVocabulary::containsAndFind_lookUpByIdentity()
{
    Vocabulary vocabulary;
    vocabulary.insert(makeUnit(QStringLiteral("Give Up"), QStringLiteral("放弃")));

    QVERIFY(vocabulary.contains(makeUnit(QStringLiteral("give up"), QStringLiteral(" 放弃 "))));
    QVERIFY(!vocabulary.contains(makeUnit(QStringLiteral("give up"), QStringLiteral("投降"))));

    const std::optional<ExpressionUnit> found =
            vocabulary.find(makeUnit(QStringLiteral("GIVE UP"), QStringLiteral("放弃")));
    QVERIFY(found.has_value());
    QCOMPARE(found->form(), QStringLiteral("Give Up")); // 取回的是库里存的那一份

    // 键是身份对象：值与它的两半顺序都钉死
    QVERIFY(found->identityKey() == ExpressionUnitKey(QStringLiteral("give up"), QStringLiteral("放弃")));

    // 身份不存在：回空而不是别的
    QVERIFY(!vocabulary.find(makeUnit(QStringLiteral("give up"), QStringLiteral("投降"))).has_value());
}

void TestVocabulary::all_returnsEveryEntryInStableOrder()
{
    Vocabulary vocabulary;
    vocabulary.insert(makeUnit(QStringLiteral("run"), QStringLiteral("跑")));
    vocabulary.insert(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")));

    const QList<ExpressionUnit> all = vocabulary.all();
    QCOMPARE(all.size(), 2);
    // 按身份排序 → "give up" 在 "run" 前
    QCOMPARE(all.at(0).normalizedForm(), QStringLiteral("give up"));
    QCOMPARE(all.at(1).normalizedForm(), QStringLiteral("run"));
}

QTEST_MAIN(TestVocabulary)
#include "tst_Vocabulary.moc"
