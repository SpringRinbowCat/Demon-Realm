#include "Application/BattleController.hpp"

#include <algorithm>
#include <utility>

#include "Domain/State/HeroState.hpp"

namespace DemonRealm
{
namespace
{

/// 在英雄状态列表中按 id 查找。
/// 参数 heroes：英雄状态列表。
/// 参数 heroId：英雄配置 id。
/// 返回值：找到返回对应状态的指针，否则返回 nullptr。
const HeroState* findHeroState(const std::vector<HeroState>& heroes, const std::string& heroId)
{
    const auto found = std::find_if(heroes.begin(),
                                    heroes.end(),
                                    [&heroId](const HeroState& hero) { return hero.getHeroId() == heroId; });
    return found == heroes.end() ? nullptr : &(*found);
}

/// 按英雄状态与展示信息构造英雄快照。
/// 参数 presentation：英雄展示信息。
/// 参数 heroState：英雄运行时状态。
/// 返回值：填充完成的英雄快照。
BattleHeroSnapshot buildHeroSnapshot(const BattleHeroPresentation& presentation, const HeroState& heroState)
{
    BattleHeroSnapshot snapshot;
    snapshot.displayName = presentation.displayName;
    snapshot.level = heroState.getLevel();
    snapshot.attack = heroState.getAttack().toString();
    snapshot.attackIntervalSeconds = heroState.getAttackIntervalSeconds().toString();
    snapshot.cardImageFile = presentation.cardImageFile;

    // 技能系统尚未实现，这里只按等级筛选已解锁技能名用于展示；技能效果落地后，
    // 解锁判定应移入 Domain，并由技能系统向英雄写入对应修正。
    for (const HeroSkillPresentation& skill : presentation.skills)
    {
        if (skill.unlockLevel <= heroState.getLevel())
        {
            snapshot.unlockedSkillNames.push_back(skill.displayName);
        }
    }

    return snapshot;
}

}  // namespace

BattleController::BattleController(std::unique_ptr<CombatSystem> combatSystem, BattleScenePresentation presentation)
    : _combatSystem(std::move(combatSystem))
    , _presentation(std::move(presentation))
{
}

bool BattleController::advance(double deltaSeconds)
{
    const CombatTickReport report = _combatSystem->advance(deltaSeconds);
    return report.hasChanges;
}

BattleSnapshot BattleController::createSnapshot() const
{
    BattleSnapshot snapshot;
    snapshot.backgroundImageFile = _presentation.backgroundImageFile;
    snapshot.bossImageFile = _presentation.bossImageFile;
    snapshot.status = createStatusSnapshot();

    const std::vector<HeroState>& heroes = _combatSystem->getHeroes();
    for (const BattleHeroPresentation& heroPresentation : _presentation.heroes)
    {
        const HeroState* heroState = findHeroState(heroes, heroPresentation.heroId);
        if (heroState == nullptr)
        {
            // 展示信息与运行时状态不匹配属于装配错误，跳过该英雄而不是展示错误数值。
            continue;
        }

        snapshot.heroes.push_back(buildHeroSnapshot(heroPresentation, *heroState));
    }

    return snapshot;
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

}  // namespace DemonRealm
