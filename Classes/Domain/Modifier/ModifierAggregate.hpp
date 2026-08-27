#pragma once

#include "Domain/Modifier/ModifierTypes.hpp"
#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 单个目标上所有修正的聚合结果。
///
/// 职责：把任意多条修正压缩成"一个加法项 + 一个乘法项"，让最终值计算与修正条数无关。
/// 结算公式固定为 `最终值 = (基础值 + 加法项) × 乘法项`，结果向下取整到 4 位小数。
///
/// 该顺序是全局约定：先加后乘。写新 buff 时按这个语义选择运算方式，不要指望顺序可配。
///
/// 线程要求：值类型，可在任意线程使用。
class ModifierAggregate
{
public:
    /// 构造中性聚合：加法项 0，乘法项 1。
    ModifierAggregate();

    /// 累加一条修正。
    /// 参数 modifier：待累加的修正；其 target 由调用方保证与本聚合一致。
    void accumulate(const Modifier& modifier);

    /// 与另一份聚合合并，用于叠加"英雄自身修正"与"全局修正"。
    /// 参数 other：另一份聚合。
    /// 返回值：合并结果，加法项相加、乘法项相乘。
    ModifierAggregate combinedWith(const ModifierAggregate& other) const;

    /// 把聚合结果应用到基础值上。
    /// 参数 baseValue：基础值。
    /// 返回值：最终值，向下取整到 4 位小数。
    Decimal applyTo(const Decimal& baseValue) const;

private:
    /// 加法项之和。
    Decimal _flatBonus;

    /// 乘法项之积，初始为 1。
    Decimal _multiplier;
};

}  // namespace DemonRealm
