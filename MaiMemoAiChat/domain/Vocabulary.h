#pragma once

#include "domain/ExpressionUnit.h"

#include <QList>
#include <QMap>

#include <optional>

namespace mai::domain {

/**
 * @brief 词库：表达单元的集合。
 *
 * 是容器不是实体——条目即表达单元，词库没有独立于条目的数据。
 * 本集合承载"同一词形、同一义项只记一条"的判重规则（R8）。
 * 入参是**表达单元**（必填检查在 ExpressionUnit::create 里做过）：条目将来长字段，本集合一个字不用改。
 * 浏览/搜索的过滤是用例的取用方式，不在这里。
 */
class Vocabulary
{
public:
    enum class InsertResult {
        Added,
        Duplicate, // 身份已存在，未入库
    };

    /// 入库；按身份判重，已存在则不入
    InsertResult insert(const ExpressionUnit& entry);

    /// 按身份问"有没有"（补录与批量导入的入库判定）
    bool contains(const ExpressionUnit& entry) const;

    /// 按身份取条目；取回的是**库里存的那一份**（原样词形是第一次入库时的）
    std::optional<ExpressionUnit> find(const ExpressionUnit& entry) const;

    /// 全量条目，按身份排序（确定性顺序）
    QList<ExpressionUnit> all() const;

private:
    QMap<ExpressionUnitKey, ExpressionUnit> m_entries; // 键＝表达单元的身份
};

} // namespace mai::domain
