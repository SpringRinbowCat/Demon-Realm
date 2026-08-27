#include "Domain/State/HeroState.hpp"

#include <cmath>
#include <utility>

namespace DemonRealm
{
namespace
{

/// 单次推进最多结算的攻击次数，double 形式，用于与除法结果比较。
const double kMaxAttacksPerAdvanceAsDouble = static_cast<double>(HeroState::kMaxAttacksPerAdvance);

}  // namespace

HeroState::HeroState(std::string heroId,
                     int level,
                     const Decimal& baseAttack,
                     const Decimal& baseAttackIntervalSeconds)
    : _heroId(std::move(heroId))
    , _level(level)
    , _baseAttack(baseAttack)
    , _baseAttackIntervalSeconds(baseAttackIntervalSeconds)
    , _modifiers()
    , _attack(baseAttack)
    , _attackIntervalSeconds(baseAttackIntervalSeconds)
    , _attackIntervalSecondsAsDouble(baseAttackIntervalSeconds.toDouble())
    , _cachedLocalRevision(_modifiers.getRevision())
    , _cachedGlobalRevision(0)
    , _elapsedSeconds(0.0)
{
    // 初始状态没有任何修正，派生属性等于基础属性，缓存版本号与之对应。
}

void HeroState::refreshDerivedAttributes(const GlobalHeroModifiers& globalModifiers)
{
    const bool localUnchanged = _modifiers.getRevision() == _cachedLocalRevision;
    const bool globalUnchanged = globalModifiers.revision == _cachedGlobalRevision;
    if (localUnchanged && globalUnchanged)
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

void HeroState::_recalculateDerivedAttributes(const GlobalHeroModifiers& globalModifiers)
{
    const ModifierAggregate attackAggregate =
        _modifiers.getAggregate(ModifierTarget::HeroAttack).combinedWith(globalModifiers.attack);
    _attack = attackAggregate.applyTo(_baseAttack);

    const ModifierAggregate intervalAggregate =
        _modifiers.getAggregate(ModifierTarget::AttackIntervalSeconds)
            .combinedWith(globalModifiers.attackIntervalSeconds);
    _attackIntervalSeconds = intervalAggregate.applyTo(_baseAttackIntervalSeconds);
    _attackIntervalSecondsAsDouble = _attackIntervalSeconds.toDouble();

    _cachedLocalRevision = _modifiers.getRevision();
    _cachedGlobalRevision = globalModifiers.revision;
}

}  // namespace DemonRealm
