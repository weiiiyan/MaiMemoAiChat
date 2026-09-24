#pragma once

#include "domain/ExpressionUnit.h"
#include "domain/LearningState.h"

#include <QList>
#include <QMap>

namespace mai::domain {

/**
 * @brief 学习状态集：把每个学习状态按身份收在一起。
 *
 * 键＝（表达单元的身份, 维度）；未学习是缺省——没有记录即未学习，集合里不存它（R4）。
 * 集合级规则："同一（表达单元, 维度）只记一条"。
 */
class LearningStates
{
public:
    /// 取状态；无记录返回未学习
    LearningState stateOf(const ExpressionUnit& entry, Dimension dimension) const;

    /// 落状态；置为未学习即删掉记录
    void setState(const ExpressionUnit& entry, Dimension dimension, LearningState state);

    /// 该维度上全部已掌握的表达单元——"已掌握词库"（R12 用词上限的数据源）
    QList<ExpressionUnit> masteredIn(Dimension dimension) const;

private:
    struct Record
    {
        ExpressionUnit unit;
        LearningState state;
    };

    // 按维度分桶，各维度独立；桶内键＝表达单元的身份
    QMap<Dimension, QMap<ExpressionUnitKey, Record>> m_byDimension;
};

} // namespace mai::domain
