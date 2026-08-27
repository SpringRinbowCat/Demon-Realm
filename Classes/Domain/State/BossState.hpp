#pragma once

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 当前 Boss 的运行时状态。
///
/// 职责：保存最大血量与剩余血量，并结算扣血。血量不会变成负数，扣血量超过剩余血量时
/// 只扣到 0，并把"实际扣除量"返回给调用方，让金币等按实际伤害结算的产出保持一致。
///
/// 关卡推进：Boss 被击败后本类只维持"已击败"状态，切换到下一个 Boss 由关卡系统调用
/// resetTo 完成；本类不自行选择下一个 Boss。
///
/// 线程要求：非线程安全，只在推进战斗的线程使用。
class BossState
{
public:
    /// 构造空状态：最大血量与剩余血量都是 0，即处于已击败状态。
    BossState();

    /// 按最大血量构造。
    /// 参数 maxHp：最大血量。
    explicit BossState(const Decimal& maxHp);

    /// 重置为指定最大血量的满血状态，用于进入新 Boss。
    /// 参数 maxHp：新的最大血量。
    void resetTo(const Decimal& maxHp);

    /// 结算扣血。
    /// 参数 damage：本次伤害量。
    /// 返回值：实际扣除的血量；剩余血量不足时小于 damage。
    Decimal applyDamage(const Decimal& damage);

    /// 取剩余血量。
    const Decimal& getRemainingHp() const;

    /// 取最大血量。
    const Decimal& getMaxHp() const;

    /// 是否已被击败，即剩余血量为 0。
    bool isDefeated() const;

private:
    /// 最大血量。
    Decimal _maxHp;

    /// 剩余血量。
    Decimal _remainingHp;
};

}  // namespace DemonRealm
