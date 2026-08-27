#pragma once

#include <memory>
#include <vector>

#include "Domain/Combat/CombatTickReport.hpp"
#include "Domain/Modifier/ModifierCollection.hpp"
#include "Domain/Numeric/Decimal.hpp"
#include "Domain/Random/RandomSource.hpp"
#include "Domain/Skill/SkillDefinition.hpp"
#include "Domain/State/BossState.hpp"
#include "Domain/State/EconomyState.hpp"
#include "Domain/State/HeroState.hpp"

namespace DemonRealm
{

/// 战斗结算系统。
///
/// 职责：持有 Boss、经济和英雄状态，按时间推进自动攻击，并把伤害与金币结算到状态上。
/// 所有伤害来源最终都要经过本类结算，后续的点击攻击和技能伤害也应在这里加入对应入口，
/// 不允许绕过本类直接改血量或金币。
///
/// 结算顺序：先按最新修正刷新英雄派生属性，再按英雄顺序结算到期的攻击；每次攻击按
/// 「实际扣血量」结算金币；Boss 被击败后立即停止本次推进，避免对着 0 血继续产出金币。
///
/// 全局修正：`getGlobalModifiers()` 是全体英雄共享的 buff 接入点，例如全队攻击力加成、
/// 全局金币加成；只作用于单个英雄的 buff 请写到对应 `HeroState` 的修正集合。
///
/// 性能：每帧的固定开销是「每个英雄一次 double 比较」；只有真的触发攻击时才做大数运算，
/// 且同一英雄在一帧内的多次攻击会先合并成一次乘法，再做一次扣血与一次金币结算。
///
/// 线程要求：非线程安全，只在主线程推进。
class CombatSystem
{
public:
    /// 构造战斗系统。
    /// 参数 bossMaxHp：当前 Boss 的最大血量。
    /// 参数 heroes：已召唤英雄的初始状态，顺序即结算顺序。
    /// 参数 randomSource：概率类技能使用的随机源，不能为空，由本类持有。
    CombatSystem(const Decimal& bossMaxHp,
                 std::vector<HeroState> heroes,
                 std::unique_ptr<RandomSource> randomSource);

    /// 按时间推进自动攻击。
    /// 参数 deltaSeconds：距上次推进的秒数，非正数直接返回空结果。
    /// 返回值：本次推进的结算结果。
    CombatTickReport advance(double deltaSeconds);

    /// 结算一次玩家对 Boss 的点击。
    ///
    /// 点击本身不造成伤害：伤害与附带效果全部来自已解锁的点击类技能，因此没有解锁任何
    /// 点击技能时点击不会有任何结果。技能按配置顺序依次结算。
    ///
    /// 返回值：本次点击的结算结果。
    CombatTickReport resolveTapAttack();

    /// 取 Boss 剩余血量。
    const Decimal& getBossRemainingHp() const;

    /// 取当前金币余额。
    const Decimal& getGoldAmount() const;

    /// Boss 是否已被击败。
    bool isBossDefeated() const;

    /// 取英雄状态列表，供展示层读取最终攻击力与攻击间隔。
    const std::vector<HeroState>& getHeroes() const;

    /// 取全局修正集合，供 buff 系统增删作用于全体的修正。
    ModifierCollection& getGlobalModifiers();

    /// 取全局修正集合的只读引用。
    const ModifierCollection& getGlobalModifiers() const;

private:
    /// 按最新的全局修正刷新全部英雄的派生属性。
    void _refreshHeroDerivedAttributes();

    /// 结算某个英雄本次到期的全部攻击。
    /// 参数 hero：发起攻击的英雄。
    /// 参数 attackCount：到期的攻击次数，必须大于 0。
    /// 参数 report：累加结算结果的输出参数。
    void _resolveHeroAttacks(const HeroState& hero, unsigned long long attackCount, CombatTickReport& report);

    /// 把一次伤害结算到 Boss 与经济状态上。
    ///
    /// 所有伤害来源都必须经过这里，金币按实际扣血量结算，避免对着 0 血继续产出金币。
    ///
    /// 参数 damage：本次伤害量。
    /// 参数 report：累加结算结果的输出参数。
    void _applyDamage(const Decimal& damage, CombatTickReport& report);

    /// 结算一个已解锁的点击类技能。
    /// 参数 hero：技能所属英雄。
    /// 参数 skill：技能定义。
    /// 参数 report：累加结算结果的输出参数。
    void _resolveTapSkill(HeroState& hero, const SkillDefinition& skill, CombatTickReport& report);

    /// 结算永久攻击力成长效果：先掷点，命中后按等级公式提升攻击力。
    /// 参数 hero：技能所属英雄。
    /// 参数 effect：效果参数。
    /// 参数 report：累加结算结果的输出参数。
    void _resolveAttackGrowth(HeroState& hero,
                              const SkillAttackGrowthEffect& effect,
                              CombatTickReport& report);

    /// Boss 状态。
    BossState _bossState;

    /// 经济状态。
    EconomyState _economyState;

    /// 已召唤英雄状态，顺序即结算顺序。
    std::vector<HeroState> _heroes;

    /// 作用于全体英雄与全局产出的修正集合。
    ModifierCollection _globalModifiers;

    /// 概率类技能使用的随机源。
    std::unique_ptr<RandomSource> _randomSource;
};

}  // namespace DemonRealm
