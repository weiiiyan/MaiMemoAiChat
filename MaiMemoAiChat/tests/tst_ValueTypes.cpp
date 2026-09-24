#include "domain/LearningState.h"
#include "domain/Rating.h"

#include <QTest>

#include <optional>

using mai::domain::Dimension;
using mai::domain::LearningState;
using mai::domain::Rating;

class TestValueTypes : public QObject
{
    Q_OBJECT

private slots:
    void dimensionVocabulary_roundTrips();
    void learningStateVocabulary_roundTrips();
    void parse_rejectsUnknownText();
    void vocabulary_spellsExactContractStrings();
    void rating_ordinalsAreFixed();
};

void TestValueTypes::dimensionVocabulary_roundTrips()
{
    const QList<Dimension> all{Dimension::Reading, Dimension::Listening,
                               Dimension::Speaking, Dimension::Writing};

    for (const Dimension dimension : all) {
        const std::optional<Dimension> parsed =
                mai::domain::parseDimension(mai::domain::toString(dimension));
        QVERIFY(parsed.has_value());
        QVERIFY(*parsed == dimension);
    }
}

void TestValueTypes::learningStateVocabulary_roundTrips()
{
    const QList<LearningState> all{LearningState::NotLearned, LearningState::InReview,
                                   LearningState::Mastered};

    for (const LearningState state : all) {
        const std::optional<LearningState> parsed =
                mai::domain::parseLearningState(mai::domain::toString(state));
        QVERIFY(parsed.has_value());
        QVERIFY(*parsed == state);
    }
}

void TestValueTypes::parse_rejectsUnknownText()
{
    QVERIFY(!mai::domain::parseDimension(QStringLiteral("reading ")).has_value());
    QVERIFY(!mai::domain::parseDimension(QStringLiteral("Reading")).has_value());
    QVERIFY(!mai::domain::parseDimension(QString()).has_value());
    QVERIFY(!mai::domain::parseLearningState(QStringLiteral("master")).has_value());
    QVERIFY(!mai::domain::parseLearningState(QStringLiteral("inreview")).has_value());
    QVERIFY(!mai::domain::parseLearningState(QString()).has_value());
}

void TestValueTypes::vocabulary_spellsExactContractStrings()
{
    // 这 7 个字面量是持久化/接线契约，必须逐条钉死：
    // 纯 round-trip 对任意一一对应的改名都成立，钉不住契约。
    QCOMPARE(mai::domain::toString(Dimension::Reading), QStringLiteral("reading"));
    QCOMPARE(mai::domain::toString(Dimension::Listening), QStringLiteral("listening"));
    QCOMPARE(mai::domain::toString(Dimension::Speaking), QStringLiteral("speaking"));
    QCOMPARE(mai::domain::toString(Dimension::Writing), QStringLiteral("writing"));

    QCOMPARE(mai::domain::toString(LearningState::NotLearned), QStringLiteral("notLearned"));
    QCOMPARE(mai::domain::toString(LearningState::InReview), QStringLiteral("inReview"));
    QCOMPARE(mai::domain::toString(LearningState::Mastered), QStringLiteral("mastered"));
}

void TestValueTypes::rating_ordinalsAreFixed()
{
    // 档位集合固定为三档（R7）
    QCOMPARE(static_cast<int>(Rating::Forgot), 0);
    QCOMPARE(static_cast<int>(Rating::Hard), 1);
    QCOMPARE(static_cast<int>(Rating::Good), 2);
}

QTEST_MAIN(TestValueTypes)
#include "tst_ValueTypes.moc"
