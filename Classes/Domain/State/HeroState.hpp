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

/// 攻击力等级的分段成长倍率。
struct AttackLevelMultiplierRange
{
    /// 区间起始等级，含边界。
    int minLevel = 0;

    /// 区间结束等级，含边界。
    int maxLevel = 0;

    /// 该区间内每级的增量倍率。
    Decimal multiplier;
};

/// 英雄状态的初始数据。
///
/// 字段较多且都来自配置，用一个结构体传入，避免构造函数出现一长串同类型参数导致传错顺序。
struct HeroSetup
{
    /// 英雄配置 id。
    std::string heroId;

    /// 初始英雄等级。
    int heroLevel = 0;

    /// 初始攻击力等级。
    int attackLevel = 0;

    /// 初始攻击速度等级。
    int attackIntervalLevel = 0;

    /// 基础攻击力。
    Decimal baseAttack;

    /// 基础攻击间隔，单位秒；必须大于 0，否则该英雄不会攻击。
    Decimal baseAttackIntervalSeconds;

    /// 攻击力从 1 级升到 2 级的增量。
    Decimal attackUpgradeBaseGain;

    /// 首次升级所需金币；攻击力与攻击速度共用一条费用序列，从这个值起算。
    Decimal firstUpgradeGoldCost;

    /// 每次升级后费用的增长倍率。
    Decimal upgradeCostMultiplier;

    /// 攻击力等级的分段成长倍率，需从 1 级开始且连续。
    std::vector<AttackLevelMultiplierRange> attackLevelMultiplierRanges;

    /// 该英雄的全部技能定义，按配置顺序。
    std::vector<SkillDefinition> skills;
};

/// 单个已召唤英雄的运行时状态。
///
/// 职责：保存英雄的基础属性、成长与升级进度、自身修正和自动攻击计时，并给出结算所需的
/// 最终属性。等级、攻击力和攻击间隔的变化由升级系统调用本类的 apply 方法完成，本类只
/// 负责按既定公式改自己的数据，不判断金币够不够，也不决定玩家能否升级。
///
/// 三种等级：英雄等级、攻击力等级、攻击速度等级各自独立。攻击力或攻击速度任一项升级，
/// 英雄等级都会加一；技能解锁按英雄等级判定。
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
    /// 参数 setup：初始数据，来自配置。
    explicit HeroState(HeroSetup setup);

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

    /// 取当前攻击速度等级。
    int getAttackIntervalLevel() const;

    /// 取下一次攻击力升级带来的攻击力增量。
    const Decimal& getNextAttackUpgradeGain() const;

    /// 取下一次攻击速度升级会缩短的秒数。
    ///
    /// 固定为当前基础攻击间隔的 1%，向下取整到 4 位小数；间隔已经小到 1% 不足最小精度时
    /// 返回 0，此时升级不会产生任何效果。
    Decimal getNextAttackIntervalReduction() const;

    /// 取下一次升级所需金币。
    ///
    /// 费用只跟英雄等级走，攻击力与攻击速度共用同一条费用序列：无论升哪一项，英雄等级都会
    /// 加一，下一次的费用也跟着按升级费用倍率递增。
    const Decimal& getUpgradeGoldCost() const;

    /// 应用一次攻击力升级。
    ///
    /// 基础攻击力增加当前增量，攻击力等级与英雄等级各加一，下一次的增量按当前攻击力等级
    /// 所在区间的倍率累乘，下一次的费用按升级费用倍率递增。
    void applyAttackUpgrade();

    /// 应用一次攻击速度升级。
    ///
    /// 基础攻击间隔减少自身的 1%，攻击速度等级与英雄等级各加一，下一次的费用按升级费用
    /// 倍率递增。
    void applyAttackIntervalUpgrade();

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

    /// 取指定攻击力等级所在区间的成长倍率。
    ///
    /// 等级超出全部区间时沿用最后一个区间的倍率：让成长突然停止更像是缺配置引起的故障，
    /// 而不是设计意图。补配置即可覆盖到更高等级。
    ///
    /// 参数 attackLevel：攻击力等级。
    /// 返回值：对应的倍率；没有配置任何区间时返回 1。
    Decimal _findAttackLevelMultiplier(int attackLevel) const;

    /// 英雄配置 id。
    std::string _heroId;

    /// 当前英雄等级。
    int _level;

    /// 当前攻击力等级。
    int _attackLevel;

    /// 当前攻击速度等级。
    int _attackIntervalLevel;

    /// 全部技能定义，按配置顺序。
    std::vector<SkillDefinition> _skills;

    /// 攻击力等级的分段成长倍率。
    std::vector<AttackLevelMultiplierRange> _attackLevelMultiplierRanges;

    /// 下一次攻击力升级的增量。
    Decimal _nextAttackUpgradeGain;

    /// 下一次升级的花费；攻击力与攻击速度共用，只随英雄等级递增。
    Decimal _upgradeGoldCost;

    /// 升级费用的增长倍率。
    Decimal _upgradeCostMultiplier;

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
