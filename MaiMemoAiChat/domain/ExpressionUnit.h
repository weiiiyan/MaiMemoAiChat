#pragma once

#include <QString>

#include <optional>

namespace mai::domain {

/**
 * @brief 表达单元的身份：（规范化词形, 规范化义项）。
 *
 * 只在表达单元里产生（ExpressionUnit::identityKey()）。比较与排序都长在本类型上，
 * 容器与调用方拿它当一个整体用，**不拆开它的两半**。
 * 构造只接受**已规范化**的值——规范化的口径在表达单元那边（normalizedForm / normalizedSense）。
 */
class ExpressionUnitKey
{
public:
    ExpressionUnitKey(QString normalizedForm, QString normalizedSense);

    bool operator==(const ExpressionUnitKey& other) const;
    bool operator<(const ExpressionUnitKey& other) const; // 供 QMap 之类的有序容器用

private:
    QString m_normalizedForm;
    QString m_normalizedSense;
};

/**
 * @brief 表达单元：词形 + 一个义项，产品的基础学习单元。
 *
 * 需求文档称"语言符号"、词库管理称"词库条目"，都是本类型。
 * 身份＝（规范化词形, 规范化义项）——多义时以义项为界，同一词形的不同义项是不同表达单元。
 * 词形与义项**必填**：只能经 create 造出来，空的造不出——领域自己兜住，不赖调用方。
 */
class ExpressionUnit
{
public:
    /// 唯一的构造入口；词形或义项为空（含纯空白）则返回空
    static std::optional<ExpressionUnit> create(const QString& form, const QString& sense);

    /// 原始词形，原样保存（词、短语或短句，句法长度不是边界）
    const QString& form() const;

    /// 义项原文（语言不限）
    const QString& sense() const;

    /// 规范化词形：忽略大小写与首尾空白
    QString normalizedForm() const;

    /// 规范化义项：只忽略首尾空白（大小写保留——义项里可能有专名）
    QString normalizedSense() const;

    /// 身份——容器的键用它；身份只有这一处定义
    ExpressionUnitKey identityKey() const;

    /// 判重：身份相同
    bool sameIdentityAs(const ExpressionUnit& other) const;

private:
    ExpressionUnit(QString form, QString sense);

    QString m_form;
    QString m_sense;
};

} // namespace mai::domain
