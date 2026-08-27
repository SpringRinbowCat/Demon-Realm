#pragma once

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 随机数来源的领域接口。
///
/// 职责：为概率类规则提供随机值。领域层只依赖这个接口，具体的播种方式与随机算法由
/// 基础设施层实现，这样概率规则可以在不依赖时钟的情况下被替换或复现。
///
/// 线程要求：由持有方在推进战斗的线程使用，实现不需要保证线程安全。
class RandomSource
{
public:
    virtual ~RandomSource() = default;

    /// 取一个 [0, 1) 区间内的随机值。
    /// 返回值：精度为 4 位小数的随机值，即 0.0000 到 0.9999 之间的一万个等概率取值。
    ///     判定概率时用 `随机值 < 概率` 的形式，概率 0.2 恰好对应 20%。
    virtual Decimal nextUnitValue() = 0;
};

}  // namespace DemonRealm
