#include "Domain/Combat/CombatSystem.hpp"

#include <utility>

namespace DemonRealm
{

CombatSystem::CombatSystem(const Decimal& bossMaxHp, std::vector<HeroState> heroes)
    : _bossState(bossMaxHp)
    , _economyState()
    , _heroes(std::move(heroes))
    , _globalModifiers()
{
}

CombatTickReport CombatSystem::advance(double deltaSeconds)
{
    CombatTickReport report;
    if (deltaSeconds <= 0.0 || _bossState.isDefeated())
    {
        return report;
    }

    _refreshHeroDerivedAttributes();

    for (HeroState& hero : _heroes)
    {
        const unsigned long long attackCount = hero.advanceAutoAttack(deltaSeconds);
        if (attackCount == 0)
        {
            continue;
        }

        _resolveHeroAttacks(hero, attackCount, report);
        if (_bossState.isDefeated())
        {
            // Boss 已死，本次推进不再结算其他英雄的攻击，避免凭空产出金币。
            // 关卡推进尚未实现，后续由关卡系统重置 Boss 状态。
            report.bossDefeatedThisTick = true;
            break;
        }
    }

    return report;
}

const Decimal& CombatSystem::getBossRemainingHp() const
{
    return _bossState.getRemainingHp();
}

const Decimal& CombatSystem::getGoldAmount() const
{
    return _economyState.getGoldAmount();
}

bool CombatSystem::isBossDefeated() const
{
    return _bossState.isDefeated();
}

const std::vector<HeroState>& CombatSystem::getHeroes() const
{
    return _heroes;
}

ModifierCollection& CombatSystem::getGlobalModifiers()
{
    return _globalModifiers;
}

const ModifierCollection& CombatSystem::getGlobalModifiers() const
{
    return _globalModifiers;
}

void CombatSystem::_refreshHeroDerivedAttributes()
{
    GlobalHeroModifiers globalModifiers;
    globalModifiers.attack = _globalModifiers.getAggregate(ModifierTarget::HeroAttack);
    globalModifiers.attackIntervalSeconds = _globalModifiers.getAggregate(ModifierTarget::AttackIntervalSeconds);
    globalModifiers.revision = _globalModifiers.getRevision();

    for (HeroState& hero : _heroes)
    {
        hero.refreshDerivedAttributes(globalModifiers);
    }
}

void CombatSystem::_resolveHeroAttacks(const HeroState& hero,
                                       unsigned long long attackCount,
                                       CombatTickReport& report)
{
    // 同一英雄在一帧内的多次攻击先合并成一次乘法，减少大数运算次数。
    const Decimal totalDamage = hero.getAttack().multiply(Decimal::fromCount(attackCount));
    if (totalDamage.isZero())
    {
        return;
    }

    const Decimal appliedDamage = _bossState.applyDamage(totalDamage);
    if (appliedDamage.isZero())
    {
        return;
    }

    const Decimal goldReward =
        _economyState.addGoldFromDamage(appliedDamage, _globalModifiers.getAggregate(ModifierTarget::GoldGain));

    report.damageDealt = report.damageDealt.add(appliedDamage);
    report.goldGained = report.goldGained.add(goldReward);
    report.hasChanges = true;
}

}  // namespace DemonRealm
