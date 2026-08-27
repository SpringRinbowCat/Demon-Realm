#include "Application/BattleController.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "Domain/State/HeroState.hpp"

namespace DemonRealm
{
namespace
{

/// 把战斗结算结果翻译成界面刷新范围。
///
/// 金币与血量只在真的产生了伤害或收益时才需要刷新，英雄栏只在英雄属性变化时才需要重建，
/// 这样空转的帧不会触发任何界面工作。
///
/// 参数 report：战斗结算结果。
/// 返回值：需要刷新的界面范围。
BattleController::RefreshRequest toRefreshRequest(const CombatTickReport& report)
{
    BattleController::RefreshRequest request;
    request.status = !report.damageDealt.isZero() || !report.goldGained.isZero();
    request.heroes = report.heroAttributesChanged;
    return request;
}

/// 把升级结果翻译成界面刷新范围。
///
/// 升级会扣金币并改变英雄属性、等级与下一次的花费，因此两块都要刷新。
///
/// 参数 outcome：升级结果。
/// 返回值：需要刷新的界面范围。
BattleController::RefreshRequest toRefreshRequest(const HeroUpgradeOutcome& outcome)
{
    BattleController::RefreshRequest request;
    request.status = outcome.applied;
    request.heroes = outcome.applied;
    return request;
}

/// 在英雄状态列表中按 id 查找序号。
/// 参数 heroes：英雄状态列表。
/// 参数 heroId：英雄配置 id。
/// 返回值：找到返回下标，否则返回列表长度。
std::size_t findHeroIndex(const std::vector<HeroState>& heroes, const std::string& heroId)
{
    for (std::size_t index = 0; index < heroes.size(); ++index)
    {
        if (heroes[index].getHeroId() == heroId)
        {
            return index;
        }
    }

    return heroes.size();
}

/// 把升级预览填进展示快照。
/// 参数 level：当前等级。
/// 参数 preview：升级预览。
/// 返回值：填充完成的升级条目快照。
BattleHeroUpgradeSnapshot buildUpgradeSnapshot(int level, const HeroUpgradePreview& preview)
{
    BattleHeroUpgradeSnapshot snapshot;
    snapshot.level = std::to_string(level);
    snapshot.costGoldAmount = preview.goldCost.toString();
    snapshot.affordable = preview.affordable;

    // 已经无法继续产生效果时不给出变化量，避免展示成"升级能减少 0 秒"。
    if (preview.effective)
    {
        snapshot.delta = preview.delta.toString();
    }

    return snapshot;
}

}  // namespace

BattleController::BattleController(std::unique_ptr<GameWorld> world,
                                   std::unique_ptr<CombatSystem> combatSystem,
                                   std::unique_ptr<HeroUpgradeSystem> upgradeSystem,
                                   BattleScenePresentation presentation)
    : _world(std::move(world))
    , _combatSystem(std::move(combatSystem))
    , _upgradeSystem(std::move(upgradeSystem))
    , _presentation(std::move(presentation))
{
}

BattleController::RefreshRequest BattleController::advance(double deltaSeconds)
{
    RefreshRequest request = toRefreshRequest(_combatSystem->advance(deltaSeconds));
    _markAffordabilityChanges(request);
    return request;
}

BattleController::RefreshRequest BattleController::onBossTapped()
{
    RefreshRequest request = toRefreshRequest(_combatSystem->resolveTapAttack());
    _markAffordabilityChanges(request);
    return request;
}

BattleController::RefreshRequest BattleController::onAttackUpgradeRequested(std::size_t heroIndex)
{
    RefreshRequest request = toRefreshRequest(_upgradeSystem->upgradeAttack(heroIndex));
    _markAffordabilityChanges(request);
    return request;
}

BattleController::RefreshRequest BattleController::onAttackIntervalUpgradeRequested(std::size_t heroIndex)
{
    RefreshRequest request = toRefreshRequest(_upgradeSystem->upgradeAttackInterval(heroIndex));
    _markAffordabilityChanges(request);
    return request;
}

void BattleController::_markAffordabilityChanges(RefreshRequest& request)
{
    const std::vector<HeroState>& heroes = _world->getHeroes();
    const Decimal& goldAmount = _world->getEconomy().getGoldAmount();
    _affordability.resize(heroes.size(), false);

    for (std::size_t index = 0; index < heroes.size(); ++index)
    {
        const bool affordable = goldAmount.compare(heroes[index].getUpgradeGoldCost()) >= 0;
        if (affordable != _affordability[index])
        {
            _affordability[index] = affordable;
            request.heroes = true;
        }
    }
}

BattleSnapshot BattleController::createSnapshot() const
{
    BattleSnapshot snapshot;
    snapshot.backgroundImageFile = _presentation.backgroundImageFile;
    snapshot.bossImageFile = _presentation.bossImageFile;
    snapshot.status = createStatusSnapshot();
    snapshot.heroes = createHeroSnapshots();
    return snapshot;
}

std::vector<BattleHeroSnapshot> BattleController::createHeroSnapshots() const
{
    std::vector<BattleHeroSnapshot> heroSnapshots;
    heroSnapshots.reserve(_presentation.heroes.size());

    const std::vector<HeroState>& heroes = _world->getHeroes();
    for (const BattleHeroPresentation& heroPresentation : _presentation.heroes)
    {
        const std::size_t heroIndex = findHeroIndex(heroes, heroPresentation.heroId);
        if (heroIndex >= heroes.size())
        {
            // 展示信息与运行时状态不匹配属于装配错误，跳过该英雄而不是展示错误数值。
            continue;
        }

        heroSnapshots.push_back(_buildHeroSnapshot(heroIndex, heroPresentation, heroes[heroIndex]));
    }

    return heroSnapshots;
}

BattleStatusSnapshot BattleController::createStatusSnapshot() const
{
    BattleStatusSnapshot snapshot;
    snapshot.bossRemainingHp = _combatSystem->getBossRemainingHp().toString();
    snapshot.goldAmount = _combatSystem->getGoldAmount().toString();
    return snapshot;
}

bool BattleController::isBossDefeated() const
{
    return _combatSystem->isBossDefeated();
}

BattleHeroSnapshot BattleController::_buildHeroSnapshot(std::size_t heroIndex,
                                                        const BattleHeroPresentation& presentation,
                                                        const HeroState& heroState) const
{
    BattleHeroSnapshot snapshot;
    snapshot.heroId = presentation.heroId;
    snapshot.displayName = presentation.displayName;
    snapshot.level = heroState.getLevel();
    snapshot.attack = heroState.getAttack().toString();
    snapshot.attackIntervalSeconds = heroState.getAttackIntervalSeconds().toString();
    snapshot.cardImageFile = presentation.cardImageFile;
    snapshot.description = presentation.description;

    // 解锁判定由领域层负责，这里只按解锁状态整理展示文字。
    for (const HeroSkillPresentation& skill : presentation.skills)
    {
        BattleHeroSkillSnapshot skillSnapshot;
        skillSnapshot.displayName = skill.displayName;
        skillSnapshot.description = skill.description;
        skillSnapshot.unlockLevel = skill.unlockLevel;
        skillSnapshot.unlocked = heroState.isSkillUnlockedById(skill.skillId);
        snapshot.skills.push_back(skillSnapshot);

        if (skillSnapshot.unlocked)
        {
            snapshot.unlockedSkillNames.push_back(skill.displayName);
        }
    }

    snapshot.attackUpgrade =
        buildUpgradeSnapshot(heroState.getAttackLevel(), _upgradeSystem->previewAttackUpgrade(heroIndex));
    snapshot.attackIntervalUpgrade = buildUpgradeSnapshot(
        heroState.getAttackIntervalLevel(), _upgradeSystem->previewAttackIntervalUpgrade(heroIndex));
    return snapshot;
}

}  // namespace DemonRealm
