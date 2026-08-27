#pragma once

#include <cstddef>
#include <string>

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 修正目标：一条修正作用在哪个属性上。
///
/// 新增 buff 类型时在 kCount 之前追加枚举项，无需改动战斗结算流程。
enum class ModifierTarget
{
    /// 英雄攻击力，直接决定单次攻击造成的伤害。
    HeroAttack = 0,

    /// 英雄攻击间隔，单位秒；缩短间隔用小于 1 的乘法系数表达。
    AttackIntervalSeconds,

    /// 金币产出，作用在"本次结算获得的金币"上。
    GoldGain,

    /// 枚举项个数，不是真实目标，仅用于按目标建索引。
    kCount
};

/// 修正目标的数量。
const std::size_t kModifierTargetCount = static_cast<std::size_t>(ModifierTarget::kCount);

/// 修正的运算方式。
enum class ModifierOperation
{
    /// 加法项：先与基础值相加。
    Add = 0,

    /// 乘法项：在加法项结算完成后相乘。
    Multiply
};

/// 一条属性修正。
///
/// 由技能、装备、公会加成等系统产出。同一来源可以产出多条修正，按 sourceId 整批移除。
struct Modifier
{
    /// 来源 id，例如技能 id 或装备 id；移除时按该 id 匹配。
    std::string sourceId;

    /// 作用目标。
    ModifierTarget target = ModifierTarget::HeroAttack;

    /// 运算方式。
    ModifierOperation operation = ModifierOperation::Add;

    /// 修正数值：加法项是绝对增量，乘法项是系数（1 表示无影响）。
    Decimal value;
};

}  // namespace DemonRealm
