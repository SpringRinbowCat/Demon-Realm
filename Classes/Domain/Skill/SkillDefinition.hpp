#pragma once

#include <string>

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 技能的触发时机。
///
/// 新增触发时机时在这里追加枚举项，并在战斗结算里补上对应的触发入口；
/// 已有技能的配置不需要改动。
enum class SkillTrigger
{
    /// 玩家点击 Boss 时触发。
    TapAttack = 0
};

/// 技能的效果类型。
enum class SkillEffectType
{
    /// 对 Boss 造成伤害，伤害量为英雄攻击力乘以倍率。
    Damage = 0,

    /// 按概率永久提升英雄攻击力，提升量与等级相关。
    PermanentAttackGrowth
};

/// 伤害类效果参数。
struct SkillDamageEffect
{
    /// 伤害倍率，作用在英雄的最终攻击力上；1 表示等同于一次普通攻击。
    Decimal attackMultiplier;
};

/// 永久攻击力成长类效果参数。
///
/// 成长量固定按 `攻击力等级 × 英雄等级 ÷ 等级乘积除数` 计算，向下取整到 4 位小数。
struct SkillAttackGrowthEffect
{
    /// 触发概率，取值范围 0 到 1；0.2 表示 20%。
    Decimal chance;

    /// 等级乘积的除数，必须大于 0。
    Decimal levelProductDivisor;
};

/// 一个技能的领域定义。
///
/// 只描述技能"做什么"与"什么时候能用"，不含展示文案：技能名等展示信息由展示数据提供，
/// 避免领域层承担文案职责。
///
/// 效果参数按 `effectType` 取用对应的成员，另一个成员的内容无意义。
struct SkillDefinition
{
    /// 技能 id，对应配置里的 `id`。
    std::string id;

    /// 解锁所需的英雄等级。
    int unlockLevel = 0;

    /// 触发时机。
    SkillTrigger trigger = SkillTrigger::TapAttack;

    /// 效果类型，决定下面哪一组参数生效。
    SkillEffectType effectType = SkillEffectType::Damage;

    /// `effectType` 为 Damage 时生效。
    SkillDamageEffect damage;

    /// `effectType` 为 PermanentAttackGrowth 时生效。
    SkillAttackGrowthEffect attackGrowth;
};

}  // namespace DemonRealm
