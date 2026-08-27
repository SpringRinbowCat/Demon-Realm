#include "Domain/Combat/CombatSystem.hpp"

#include <utility>

namespace DemonRealm
{

CombatSystem::CombatSystem(GameWorld& world, std::unique_ptr<RandomSource> randomSource)
    : _world(world)
    , _randomSource(std::move(randomSource))
{
}

CombatTickReport CombatSystem::resolveTapAttack()
{
    CombatTickReport report;
    if (_world.getBoss().isDefeated())
    {
        return report;
    }

    _world.refreshHeroDerivedAttributes();

    for (HeroState& hero : _world.getHeroes())
    {
        for (const SkillDefinition& skill : hero.getSkills())
        {
            if (skill.trigger != SkillTrigger::TapAttack || !hero.isSkillUnlocked(skill))
            {
                continue;
            }

            _resolveTapSkill(hero, skill, report);
            if (_world.getBoss().isDefeated())
            {
                report.bossDefeatedThisTick = true;
                break;
            }
        }

        if (_world.getBoss().isDefeated())
        {
            break;
        }
    }

    // 成长类效果改的是基础数值，最终属性要立刻重算，否则调用方读到的仍是本次结算前的旧值。
    if (report.heroAttributesChanged)
    {
        _world.refreshHeroDerivedAttributes();
    }

    return report;
}

CombatTickReport CombatSystem::advance(double deltaSeconds)
{
    CombatTickReport report;
    if (deltaSeconds <= 0.0 || _world.getBoss().isDefeated())
    {
        return report;
    }

    _world.refreshHeroDerivedAttributes();

    for (HeroState& hero : _world.getHeroes())
    {
        const unsigned long long attackCount = hero.advanceAutoAttack(deltaSeconds);
        if (attackCount == 0)
        {
            continue;
        }

        _resolveHeroAttacks(hero, attackCount, report);
        if (_world.getBoss().isDefeated())
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
    return _world.getBoss().getRemainingHp();
}

const Decimal& CombatSystem::getGoldAmount() const
{
    return _world.getEconomy().getGoldAmount();
}

bool CombatSystem::isBossDefeated() const
{
    return _world.getBoss().isDefeated();
}

const std::vector<HeroState>& CombatSystem::getHeroes() const
{
    return _world.getHeroes();
}

ModifierCollection& CombatSystem::getGlobalModifiers()
{
    return _world.getGlobalModifiers();
}

const ModifierCollection& CombatSystem::getGlobalModifiers() const
{
    return _world.getGlobalModifiers();
}

void CombatSystem::_resolveHeroAttacks(const HeroState& hero,
                                       unsigned long long attackCount,
                                       CombatTickReport& report)
{
    // 同一英雄在一帧内的多次攻击先合并成一次乘法，减少大数运算次数。
    _applyDamage(hero.getAttack().multiply(Decimal::fromCount(attackCount)), report);
}

void CombatSystem::_applyDamage(const Decimal& damage, CombatTickReport& report)
{
    if (damage.isZero())
    {
        return;
    }

    const Decimal appliedDamage = _world.getBoss().applyDamage(damage);
    if (appliedDamage.isZero())
    {
        return;
    }

    const Decimal goldReward =
        _world.getEconomy().addGoldFromDamage(appliedDamage, _world.getGlobalModifiers().getAggregate(ModifierTarget::GoldGain));

    report.damageDealt = report.damageDealt.add(appliedDamage);
    report.goldGained = report.goldGained.add(goldReward);
    report.hasChanges = true;
}

void CombatSystem::_resolveTapSkill(HeroState& hero, const SkillDefinition& skill, CombatTickReport& report)
{
    switch (skill.effectType)
    {
        case SkillEffectType::Damage:
            _applyDamage(hero.getAttack().multiply(skill.damage.attackMultiplier), report);
            break;

        case SkillEffectType::PermanentAttackGrowth:
            _resolveAttackGrowth(hero, skill.attackGrowth, report);
            break;
    }
}

void CombatSystem::_resolveAttackGrowth(HeroState& hero,
                                        const SkillAttackGrowthEffect& effect,
                                        CombatTickReport& report)
{
    // 每次点击独立掷点：随机源按当前时间重新播种，不累积"欠了多少次没触发"的状态，
    // 因此该效果没有保底，连续不触发与连续触发都是允许的结果。
    if (_randomSource->nextUnitValue().compare(effect.chance) >= 0)
    {
        return;
    }

    // 成长量 = 攻击力等级 × 英雄等级 ÷ 等级乘积除数，向下取整到 4 位小数。
    const Decimal levelProduct =
        Decimal::fromCount(static_cast<unsigned long long>(hero.getAttackLevel()))
            .multiply(Decimal::fromCount(static_cast<unsigned long long>(hero.getLevel())));

    Decimal growthAmount;
    if (!levelProduct.tryDivide(effect.levelProductDivisor, growthAmount) || growthAmount.isZero())
    {
        // 除数为 0 属于配置错误，装配阶段已经拦截；成长量不足最小精度时按不成长处理。
        return;
    }

    hero.addPermanentAttackBonus(growthAmount);
    report.heroAttributesChanged = true;
    report.hasChanges = true;
}

}  // namespace DemonRealm
