#pragma once

#include "Domain/Random/RandomSource.hpp"

namespace DemonRealm
{

/// 按当前时间播种的随机数来源。
///
/// 职责：每次取样都用当前的高精度时间重新播种，因此相邻两次取样之间不存在任何序列
/// 关联，也不会积累"欠了多少次没触发"的状态。概率类技能因此没有保底：连续多次不触发
/// 与连续多次触发都是允许的结果。
///
/// 为什么不复用一个长期存活的随机引擎：长期引擎会让结果依赖调用次数，虽然分布更均匀，
/// 但那等价于一种隐式保底，与"每次点击都是独立事件"的设计要求相反。
///
/// 线程要求：非线程安全，只在推进战斗的线程使用。
class TimeSeededRandomSource : public RandomSource
{
public:
    TimeSeededRandomSource();

    /// 取一个 [0, 1) 区间内的随机值，精度 4 位小数。
    Decimal nextUnitValue() override;

private:
    /// 取样序号，混入种子用于区分同一时间刻度内的连续取样。
    ///
    /// 高精度时钟在极短间隔内可能返回相同计数，只用时间会退化成同一个结果。该序号只
    /// 用来打散种子，不参与任何"多少次之内必定触发"的判断，因此不构成保底。
    unsigned long long _sampleIndex;
};

}  // namespace DemonRealm
