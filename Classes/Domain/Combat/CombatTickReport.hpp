#pragma once

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 一次战斗推进的结算结果。
///
/// 用途：让调用方知道"这次推进有没有产生变化"，从而只在必要时刷新界面，避免每帧重新
/// 格式化数值和重排文本。后续接入事件总线时，可由这份结果派发领域事件。
struct CombatTickReport
{
    /// 本次推进对 Boss 造成的实际伤害总量。
    Decimal damageDealt;

    /// 本次推进获得的金币总量。
    Decimal goldGained;

    /// 本次推进中 Boss 是否被击败。
    bool bossDefeatedThisTick = false;

    /// 英雄属性是否发生变化，例如技能永久提升了攻击力；需要刷新英雄栏。
    bool heroAttributesChanged = false;

    /// 是否有展示数值发生变化，需要刷新界面。
    bool hasChanges = false;
};

}  // namespace DemonRealm
