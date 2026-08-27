#pragma once

#include <string>
#include <vector>

#include "Domain/Modifier/ModifierAggregate.hpp"
#include "Domain/Modifier/ModifierCollection.hpp"
#include "Domain/Numeric/Decimal.hpp"
#include "Domain/Skill/SkillDefinition.hpp"

namespace DemonRealm
{

/// 作用于全体英雄的修正快照。
///
/// 由战斗系统在每次推进前算好一次，再交给每个英雄使用，避免每个英雄重复聚合全局修正。
struct GlobalHeroModifiers
{
    /// 全局攻击力修正聚合。
    ModifierAggregate attack;

    /// 全局攻击间隔修正聚合。
    ModifierAggregate attackIntervalSeconds;

    /// 产出这份快照的全局修正集合版本号，用于英雄侧的缓存失效判断。
    unsigned long long revision = 0;
};

/// 单个已召唤英雄的运行时状态。
///
/// 职责：保存英雄的基础属性、自身修正和自动攻击计时，并给出结算所需的最终属性。
/// 等级成长、技能解锁等规则由对应系统写入基础属性或修正，本类不自行推导。
///
/// 派生属性缓存：最终攻击力和最终攻击间隔只在"自身修正"或"全局修正"版本变化后重算，
/// 平时每帧只做一次 double 比较，不触碰字符串大数运算。
///
/// 线程要求：非线程安全，只在推进战斗的线程使用。
class HeroState
{
public:
    /// 单次推进最多结算的攻击次数。
    ///
    /// 用于兜底：进程被挂起很久后单帧 dt 可能极大，无上限会在一帧内结算成千上万次攻击
    /// 而卡住画面。离线期间的收益应由挂机系统单独补算，不依赖这里的追帧。
    static const unsigned long long kMaxAttacksPerAdvance = 600;

    /// 构造英雄状态。
    /// 参数 heroId：英雄配置 id。
    /// 参数 level：当前英雄等级。
    /// 参数 attackLevel：当前攻击力等级；与英雄等级是两个独立概念，升级系统尚未实现，
    ///     目前由组合根按初始值传入。
    /// 参数 baseAttack：基础攻击力。
    /// 参数 baseAttackIntervalSeconds：基础攻击间隔，单位秒；必须大于 0，否则该英雄不会攻击。
    /// 参数 skills：该英雄的全部技能定义，按配置顺序；解锁判定由本类按等级完成。
    HeroState(std::string heroId,
              int level,
              int attackLevel,
              const Decimal& baseAttack,
              const Decimal& baseAttackIntervalSeconds,
              std::vector<SkillDefinition> skills);

    /// 按最新的全局修正刷新派生属性；版本未变化时不做任何计算。
    /// 参数 globalModifiers：全局修正快照。
    void refreshDerivedAttributes(const GlobalHeroModifiers& globalModifiers);

    /// 推进自动攻击计时。
    /// 参数 deltaSeconds：距上次推进的秒数，非正数按 0 处理。
    /// 返回值：本次应结算的攻击次数，可能为 0；受 kMaxAttacksPerAdvance 限制。
    unsigned long long advanceAutoAttack(double deltaSeconds);

    /// 取英雄配置 id。
    const std::string& getHeroId() const;

    /// 取当前英雄等级。
    int getLevel() const;

    /// 取当前攻击力等级。
    int getAttackLevel() const;

    /// 取全部技能定义，按配置顺序。
    const std::vector<SkillDefinition>& getSkills() const;

    /// 判断技能是否已解锁。
    /// 参数 skill：技能定义。
    /// 返回值：解锁所需等级不高于当前英雄等级时返回 true。
    bool isSkillUnlocked(const SkillDefinition& skill) const;

    /// 按技能 id 判断技能是否已解锁。
    /// 参数 skillId：技能 id。
    /// 返回值：技能存在且已解锁时返回 true。
    bool isSkillUnlockedById(const std::string& skillId) const;

    /// 永久提升攻击力。
    ///
    /// 与修正（buff）不同，这里的提升不可移除，会一直计入基础攻击力之后的最终攻击力。
    /// 供成长类技能使用。
    ///
    /// 参数 amount：提升量；为 0 时不做任何改动。
    void addPermanentAttackBonus(const Decimal& amount);

    /// 取累计的永久攻击力提升量。
    const Decimal& getPermanentAttackBonus() const;

    /// 取最终攻击力，即单次攻击造成的伤害。
    /// 返回值：已计入自身与全局修正的攻击力。
    const Decimal& getAttack() const;

    /// 取最终攻击间隔，单位秒。
    const Decimal& getAttackIntervalSeconds() const;

    /// 取自身修正集合，供技能、装备等系统增删 buff。
    ModifierCollection& getModifiers();

    /// 取自身修正集合的只读引用。
    const ModifierCollection& getModifiers() const;

private:
    /// 重算最终攻击力与最终攻击间隔。
    /// 参数 globalModifiers：全局修正快照。
    void _recalculateDerivedAttributes(const GlobalHeroModifiers& globalModifiers);

    /// 英雄配置 id。
    std::string _heroId;

    /// 当前英雄等级。
    int _level;

    /// 当前攻击力等级。
    int _attackLevel;

    /// 全部技能定义，按配置顺序。
    std::vector<SkillDefinition> _skills;

    /// 基础攻击力，不含任何修正。
    Decimal _baseAttack;

    /// 累计的永久攻击力提升量，参与最终攻击力计算且不可移除。
    Decimal _permanentAttackBonus;

    /// 基础攻击间隔，单位秒，不含任何修正。
    Decimal _baseAttackIntervalSeconds;

    /// 英雄自身的修正集合。
    ModifierCollection _modifiers;

    /// 最终攻击力缓存。
    Decimal _attack;

    /// 最终攻击间隔缓存。
    Decimal _attackIntervalSeconds;

    /// 最终攻击间隔的 double 形式，只用于每帧的计时比较。
    double _attackIntervalSecondsAsDouble;

    /// 生成当前缓存时的自身修正版本号。
    unsigned long long _cachedLocalRevision;

    /// 生成当前缓存时的全局修正版本号。
    unsigned long long _cachedGlobalRevision;

    /// 派生属性缓存是否已失效；永久成长等不经过修正集合的改动会置上该标记。
    bool _derivedAttributesDirty;

    /// 距上次攻击已累计的秒数。
    double _elapsedSeconds;
};

}  // namespace DemonRealm
