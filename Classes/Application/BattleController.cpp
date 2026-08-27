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

    // 解锁判定由领域层负责，这里只把已解锁技能的展示名取出来。
    for (const HeroSkillPresentation& skill : presentation.skills)
    {
        if (heroState.isSkillUnlockedById(skill.skillId))
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

BattleController::RefreshRequest BattleController::advance(double deltaSeconds)
{
    return toRefreshRequest(_combatSystem->advance(deltaSeconds));
}

BattleController::RefreshRequest BattleController::onBossTapped()
{
    return toRefreshRequest(_combatSystem->resolveTapAttack());
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

    const std::vector<HeroState>& heroes = _combatSystem->getHeroes();
    for (const BattleHeroPresentation& heroPresentation : _presentation.heroes)
    {
        const HeroState* heroState = findHeroState(heroes, heroPresentation.heroId);
        if (heroState == nullptr)
        {
            // 展示信息与运行时状态不匹配属于装配错误，跳过该英雄而不是展示错误数值。
            continue;
        }

        heroSnapshots.push_back(buildHeroSnapshot(heroPresentation, *heroState));
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

}  // namespace DemonRealm
