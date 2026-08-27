#include "Domain/State/HeroState.hpp"

#include <cmath>
#include <utility>

namespace DemonRealm
{
namespace
{

/// 单次推进最多结算的攻击次数，double 形式，用于与除法结果比较。
const double kMaxAttacksPerAdvanceAsDouble = static_cast<double>(HeroState::kMaxAttacksPerAdvance);

/// 攻击速度升级时缩短比例的分母：每次缩短当前攻击间隔的 1%。
///
/// 该比例是固定规则而非可调数值，因此留在领域层而不是配置里；若将来需要按英雄区分，
/// 再移到配置并通过 HeroSetup 传入。
const unsigned long long kAttackIntervalReductionDivisor = 100;

/// 攻击速度升级时缩短的比例，即 1%。
Decimal attackIntervalReductionRate()
{
    Decimal rate;
    // 1/100 在 4 位小数下可以精确表示，除数是常量也不会为 0，因此不会失败。
    static_cast<void>(
        Decimal::fromCount(1).tryDivide(Decimal::fromCount(kAttackIntervalReductionDivisor), rate));
    return rate;
}

}  // namespace

HeroState::HeroState(HeroSetup setup)
    : _heroId(std::move(setup.heroId))
    , _level(setup.heroLevel)
    , _attackLevel(setup.attackLevel)
    , _attackIntervalLevel(setup.attackIntervalLevel)
    , _skills(std::move(setup.skills))
    , _attackLevelMultiplierRanges(std::move(setup.attackLevelMultiplierRanges))
    , _nextAttackUpgradeGain(setup.attackUpgradeBaseGain)
    , _upgradeGoldCost(setup.firstUpgradeGoldCost)
    , _upgradeCostMultiplier(setup.upgradeCostMultiplier)
    , _baseAttack(setup.baseAttack)
    , _permanentAttackBonus(Decimal::zero())
    , _baseAttackIntervalSeconds(setup.baseAttackIntervalSeconds)
    , _modifiers()
    , _attack(setup.baseAttack)
    , _attackIntervalSeconds(setup.baseAttackIntervalSeconds)
    , _attackIntervalSecondsAsDouble(setup.baseAttackIntervalSeconds.toDouble())
    , _cachedLocalRevision(_modifiers.getRevision())
    , _cachedGlobalRevision(0)
    , _derivedAttributesDirty(false)
    , _elapsedSeconds(0.0)
{
    // 初始状态没有任何修正与永久成长，派生属性等于基础属性，缓存版本号与之对应。
}

void HeroState::refreshDerivedAttributes(const GlobalHeroModifiers& globalModifiers)
{
    const bool localUnchanged = _modifiers.getRevision() == _cachedLocalRevision;
    const bool globalUnchanged = globalModifiers.revision == _cachedGlobalRevision;
    if (!_derivedAttributesDirty && localUnchanged && globalUnchanged)
    {
        return;
    }

    _recalculateDerivedAttributes(globalModifiers);
}

unsigned long long HeroState::advanceAutoAttack(double deltaSeconds)
{
    // 攻击间隔为 0 时无法定义攻击频率，直接视为不攻击，避免出现无限次结算。
    if (_attackIntervalSecondsAsDouble <= 0.0 || deltaSeconds <= 0.0)
    {
        return 0;
    }

    _elapsedSeconds += deltaSeconds;
    if (_elapsedSeconds < _attackIntervalSecondsAsDouble)
    {
        return 0;
    }

    double dueCount = std::floor(_elapsedSeconds / _attackIntervalSecondsAsDouble);
    if (dueCount > kMaxAttacksPerAdvanceAsDouble)
    {
        // 超出上限的部分直接丢弃，并清空计时，避免累积成越来越长的补算队列。
        dueCount = kMaxAttacksPerAdvanceAsDouble;
        _elapsedSeconds = 0.0;
    }
    else
    {
        _elapsedSeconds -= dueCount * _attackIntervalSecondsAsDouble;
    }

    return static_cast<unsigned long long>(dueCount);
}

const std::string& HeroState::getHeroId() const
{
    return _heroId;
}

int HeroState::getLevel() const
{
    return _level;
}

int HeroState::getAttackLevel() const
{
    return _attackLevel;
}

int HeroState::getAttackIntervalLevel() const
{
    return _attackIntervalLevel;
}

const Decimal& HeroState::getNextAttackUpgradeGain() const
{
    return _nextAttackUpgradeGain;
}

Decimal HeroState::getNextAttackIntervalReduction() const
{
    // 按基础攻击间隔计算，而不是含 buff 的最终间隔：升级改的是基础值，buff 由修正另行叠加，
    // 否则一个临时 buff 会永久改变成长曲线。
    return _baseAttackIntervalSeconds.multiply(attackIntervalReductionRate());
}

const Decimal& HeroState::getUpgradeGoldCost() const
{
    return _upgradeGoldCost;
}

void HeroState::applyAttackUpgrade()
{
    _baseAttack = _baseAttack.add(_nextAttackUpgradeGain);
    ++_attackLevel;
    ++_level;

    // 下一次的增量按新的攻击力等级所在区间累乘：等级 1 升 2 用配置的基础增量，
    // 等级 2 升 3 起才开始乘倍率。
    _nextAttackUpgradeGain = _nextAttackUpgradeGain.multiply(_findAttackLevelMultiplier(_attackLevel));
    _upgradeGoldCost = _upgradeGoldCost.multiply(_upgradeCostMultiplier);
    _derivedAttributesDirty = true;
}

void HeroState::applyAttackIntervalUpgrade()
{
    _baseAttackIntervalSeconds =
        _baseAttackIntervalSeconds.subtractClampedToZero(getNextAttackIntervalReduction());
    ++_attackIntervalLevel;
    ++_level;

    _upgradeGoldCost = _upgradeGoldCost.multiply(_upgradeCostMultiplier);
    _derivedAttributesDirty = true;
}

const std::vector<SkillDefinition>& HeroState::getSkills() const
{
    return _skills;
}

bool HeroState::isSkillUnlocked(const SkillDefinition& skill) const
{
    return skill.unlockLevel <= _level;
}

bool HeroState::isSkillUnlockedById(const std::string& skillId) const
{
    for (const SkillDefinition& skill : _skills)
    {
        if (skill.id == skillId)
        {
            return isSkillUnlocked(skill);
        }
    }

    return false;
}

void HeroState::addPermanentAttackBonus(const Decimal& amount)
{
    if (amount.isZero())
    {
        return;
    }

    _permanentAttackBonus = _permanentAttackBonus.add(amount);
    // 永久成长不走修正集合，版本号不会变化，因此显式标记派生属性需要重算。
    _derivedAttributesDirty = true;
}

const Decimal& HeroState::getPermanentAttackBonus() const
{
    return _permanentAttackBonus;
}

const Decimal& HeroState::getAttack() const
{
    return _attack;
}

const Decimal& HeroState::getAttackIntervalSeconds() const
{
    return _attackIntervalSeconds;
}

ModifierCollection& HeroState::getModifiers()
{
    return _modifiers;
}

const ModifierCollection& HeroState::getModifiers() const
{
    return _modifiers;
}

Decimal HeroState::_findAttackLevelMultiplier(int attackLevel) const
{
    if (_attackLevelMultiplierRanges.empty())
    {
        return Decimal::one();
    }

    for (const AttackLevelMultiplierRange& range : _attackLevelMultiplierRanges)
    {
        if (attackLevel >= range.minLevel && attackLevel <= range.maxLevel)
        {
            return range.multiplier;
        }
    }

    // 超出全部区间时沿用最后一个区间的倍率，等级更高时补配置即可覆盖。
    return _attackLevelMultiplierRanges.back().multiplier;
}

void HeroState::_recalculateDerivedAttributes(const GlobalHeroModifiers& globalModifiers)
{
    // 永久成长先并入基础攻击力，再套用修正，因此攻击力加成对成长后的数值同样生效。
    const ModifierAggregate attackAggregate =
        _modifiers.getAggregate(ModifierTarget::HeroAttack).combinedWith(globalModifiers.attack);
    _attack = attackAggregate.applyTo(_baseAttack.add(_permanentAttackBonus));

    const ModifierAggregate intervalAggregate =
        _modifiers.getAggregate(ModifierTarget::AttackIntervalSeconds)
            .combinedWith(globalModifiers.attackIntervalSeconds);
    _attackIntervalSeconds = intervalAggregate.applyTo(_baseAttackIntervalSeconds);
    _attackIntervalSecondsAsDouble = _attackIntervalSeconds.toDouble();

    _cachedLocalRevision = _modifiers.getRevision();
    _cachedGlobalRevision = globalModifiers.revision;
    _derivedAttributesDirty = false;
}

}  // namespace DemonRealm
