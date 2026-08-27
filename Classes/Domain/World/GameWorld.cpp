#include "Domain/World/GameWorld.hpp"

#include <utility>

namespace DemonRealm
{

GameWorld::GameWorld(const Decimal& bossMaxHp, std::vector<HeroState> heroes)
    : _boss(bossMaxHp)
    , _economy()
    , _heroes(std::move(heroes))
    , _globalModifiers()
{
}

BossState& GameWorld::getBoss()
{
    return _boss;
}

const BossState& GameWorld::getBoss() const
{
    return _boss;
}

EconomyState& GameWorld::getEconomy()
{
    return _economy;
}

const EconomyState& GameWorld::getEconomy() const
{
    return _economy;
}

std::vector<HeroState>& GameWorld::getHeroes()
{
    return _heroes;
}

const std::vector<HeroState>& GameWorld::getHeroes() const
{
    return _heroes;
}

void GameWorld::refreshHeroDerivedAttributes()
{
    // 全局修正只聚合一次，再交给每个英雄使用，避免每个英雄重复聚合。
    GlobalHeroModifiers globalModifiers;
    globalModifiers.attack = _globalModifiers.getAggregate(ModifierTarget::HeroAttack);
    globalModifiers.attackIntervalSeconds =
        _globalModifiers.getAggregate(ModifierTarget::AttackIntervalSeconds);
    globalModifiers.revision = _globalModifiers.getRevision();

    for (HeroState& hero : _heroes)
    {
        hero.refreshDerivedAttributes(globalModifiers);
    }
}

ModifierCollection& GameWorld::getGlobalModifiers()
{
    return _globalModifiers;
}

const ModifierCollection& GameWorld::getGlobalModifiers() const
{
    return _globalModifiers;
}

}  // namespace DemonRealm
