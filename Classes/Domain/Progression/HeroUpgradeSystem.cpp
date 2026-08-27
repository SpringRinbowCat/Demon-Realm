#include "Domain/Progression/HeroUpgradeSystem.hpp"

namespace DemonRealm
{
namespace
{

/// 按变化量、费用与当前金币组装预览信息。
/// 参数 delta：升级带来的变化量。
/// 参数 goldCost：升级所需金币。
/// 参数 goldAmount：当前金币余额。
/// 返回值：填充完成的预览信息。
HeroUpgradePreview buildPreview(const Decimal& delta, const Decimal& goldCost, const Decimal& goldAmount)
{
    HeroUpgradePreview preview;
    preview.delta = delta;
    preview.goldCost = goldCost;
    preview.affordable = goldAmount.compare(goldCost) >= 0;
    preview.effective = !delta.isZero();
    return preview;
}

}  // namespace

HeroUpgradeSystem::HeroUpgradeSystem(GameWorld& world)
    : _world(world)
{
}

HeroUpgradePreview HeroUpgradeSystem::previewAttackUpgrade(std::size_t heroIndex) const
{
    const HeroState* hero = _findHero(heroIndex);
    if (hero == nullptr)
    {
        return HeroUpgradePreview();
    }

    return buildPreview(hero->getNextAttackUpgradeGain(),
                        hero->getUpgradeGoldCost(),
                        _world.getEconomy().getGoldAmount());
}

HeroUpgradePreview HeroUpgradeSystem::previewAttackIntervalUpgrade(std::size_t heroIndex) const
{
    const HeroState* hero = _findHero(heroIndex);
    if (hero == nullptr)
    {
        return HeroUpgradePreview();
    }

    return buildPreview(hero->getNextAttackIntervalReduction(),
                        hero->getUpgradeGoldCost(),
                        _world.getEconomy().getGoldAmount());
}

HeroUpgradeOutcome HeroUpgradeSystem::upgradeAttack(std::size_t heroIndex)
{
    HeroUpgradeOutcome outcome;
    HeroState* hero = _findHero(heroIndex);
    if (hero == nullptr)
    {
        return outcome;
    }

    const Decimal goldCost = hero->getUpgradeGoldCost();
    if (!_world.getEconomy().trySpendGold(goldCost))
    {
        // 金币不足时不扣钱也不升级，由界面提示玩家。
        return outcome;
    }

    hero->applyAttackUpgrade();
    // 升级改的是基础属性，最终属性要立刻重算，否则外部会读到升级前的旧值。
    _world.refreshHeroDerivedAttributes();
    outcome.applied = true;
    outcome.spentGold = goldCost;
    return outcome;
}

HeroUpgradeOutcome HeroUpgradeSystem::upgradeAttackInterval(std::size_t heroIndex)
{
    HeroUpgradeOutcome outcome;
    HeroState* hero = _findHero(heroIndex);
    if (hero == nullptr)
    {
        return outcome;
    }

    if (hero->getNextAttackIntervalReduction().isZero())
    {
        // 攻击间隔已经小到 1% 不足最小精度，升级不会有任何效果，不能让玩家白花钱。
        return outcome;
    }

    const Decimal goldCost = hero->getUpgradeGoldCost();
    if (!_world.getEconomy().trySpendGold(goldCost))
    {
        return outcome;
    }

    hero->applyAttackIntervalUpgrade();
    _world.refreshHeroDerivedAttributes();
    outcome.applied = true;
    outcome.spentGold = goldCost;
    return outcome;
}

HeroState* HeroUpgradeSystem::_findHero(std::size_t heroIndex)
{
    std::vector<HeroState>& heroes = _world.getHeroes();
    if (heroIndex >= heroes.size())
    {
        return nullptr;
    }

    return &heroes[heroIndex];
}

const HeroState* HeroUpgradeSystem::_findHero(std::size_t heroIndex) const
{
    const std::vector<HeroState>& heroes = _world.getHeroes();
    if (heroIndex >= heroes.size())
    {
        return nullptr;
    }

    return &heroes[heroIndex];
}

}  // namespace DemonRealm
