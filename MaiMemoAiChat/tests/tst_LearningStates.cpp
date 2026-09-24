#include "domain/LearningStates.h"
#include "TestUnits.h"

#include <QTest>

using mai::domain::Dimension;
using mai::domain::ExpressionUnit;
using mai::domain::LearningState;
using mai::domain::LearningStates;
using mai::test::makeUnit;

class TestLearningStates : public QObject
{
    Q_OBJECT

private slots:
    void stateOf_defaultsToNotLearned();
    void setState_storesAndOverwrites();
    void setState_backToNotLearned_removesTheRecord();
    void masteredIn_isPerDimension();
    void masteredIn_listsOnlyMasteredEntries();
    void keys_ignoreCaseAndSurroundingWhitespaceOfTheForm();
    void keys_distinguishSensesOfTheSameForm();
};

void TestLearningStates::stateOf_defaultsToNotLearned()
{
    const LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));

    QVERIFY(states.stateOf(giveUp, Dimension::Reading) == LearningState::NotLearned);
}

void TestLearningStates::setState_storesAndOverwrites()
{
    LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));

    states.setState(giveUp, Dimension::Reading, LearningState::InReview);
    QVERIFY(states.stateOf(giveUp, Dimension::Reading) == LearningState::InReview);

    states.setState(giveUp, Dimension::Reading, LearningState::Mastered);
    QVERIFY(states.stateOf(giveUp, Dimension::Reading) == LearningState::Mastered);
}

void TestLearningStates::setState_backToNotLearned_removesTheRecord()
{
    LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));
    states.setState(giveUp, Dimension::Reading, LearningState::Mastered);

    states.setState(giveUp, Dimension::Reading, LearningState::NotLearned);

    QVERIFY(states.stateOf(giveUp, Dimension::Reading) == LearningState::NotLearned);
    QVERIFY(states.masteredIn(Dimension::Reading).isEmpty());
}

void TestLearningStates::masteredIn_isPerDimension()
{
    LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));
    states.setState(giveUp, Dimension::Reading, LearningState::Mastered);

    QCOMPARE(states.masteredIn(Dimension::Reading).size(), 1);
    // 各维度独立：读维度的标记在听维度看不见
    QVERIFY(states.masteredIn(Dimension::Listening).isEmpty());
    QVERIFY(states.stateOf(giveUp, Dimension::Listening) == LearningState::NotLearned);
}

void TestLearningStates::masteredIn_listsOnlyMasteredEntries()
{
    LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));
    const ExpressionUnit run = makeUnit(QStringLiteral("run"), QStringLiteral("跑"));
    states.setState(giveUp, Dimension::Reading, LearningState::InReview);
    states.setState(run, Dimension::Reading, LearningState::Mastered);

    const QList<ExpressionUnit> mastered = states.masteredIn(Dimension::Reading);

    QCOMPARE(mastered.size(), 1);
    QCOMPARE(mastered.at(0).form(), QStringLiteral("run"));
}

void TestLearningStates::keys_ignoreCaseAndSurroundingWhitespaceOfTheForm()
{
    LearningStates states;
    const ExpressionUnit giveUp = makeUnit(QStringLiteral("give up"), QStringLiteral("放弃"));
    const ExpressionUnit shouty = makeUnit(QStringLiteral("  GIVE UP "), QStringLiteral(" 放弃 "));
    states.setState(giveUp, Dimension::Reading, LearningState::Mastered);

    QVERIFY(states.stateOf(shouty, Dimension::Reading) == LearningState::Mastered);
}

void TestLearningStates::keys_distinguishSensesOfTheSameForm()
{
    LearningStates states;
    states.setState(makeUnit(QStringLiteral("give up"), QStringLiteral("放弃")),
                    Dimension::Reading, LearningState::Mastered);

    // 身份含义项：同一词形的另一个义项各算各的，标记不串
    QVERIFY(states.stateOf(makeUnit(QStringLiteral("give up"), QStringLiteral("投降")),
                           Dimension::Reading) == LearningState::NotLearned);
    QCOMPARE(states.masteredIn(Dimension::Reading).size(), 1);
}

QTEST_MAIN(TestLearningStates)
#include "tst_LearningStates.moc"
