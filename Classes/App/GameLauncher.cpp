#include "App/GameLauncher.hpp"

#include "cocos2d.h"

#include <memory>
#include <utility>
#include <vector>

#include "Application/BattlePresentationData.hpp"
#include "Domain/Combat/CombatSystem.hpp"
#include "Domain/Numeric/Decimal.hpp"
#include "Domain/Progression/HeroUpgradeSystem.hpp"
#include "Domain/Random/RandomSource.hpp"
#include "Domain/Skill/SkillDefinition.hpp"
#include "Domain/State/HeroState.hpp"
#include "Domain/World/GameWorld.hpp"
#include "Infrastructure/Config/ConfigService.hpp"
#include "Infrastructure/Random/TimeSeededRandomSource.hpp"
#include "Presentation/MainSceneView.hpp"

namespace DemonRealm
{
namespace
{

/// 攻击力与攻击速度的初始等级；两者与英雄等级各自独立计数。
const int kInitialAttackLevel = 1;
const int kInitialAttackIntervalLevel = 1;

/// 配置里支持的技能触发时机标识。
const char* const kTapAttackTrigger = "tapAttack";

/// 配置里支持的技能效果类型标识。
const char* const kDamageEffectType = "damage";
const char* const kPermanentAttackGrowthEffectType = "permanentAttackGrowth";

/// 设计分辨率宽度，与像素资源的 540x960 竖屏基准一致。
const float kDesignResolutionWidth = 540.0F;

/// 设计分辨率高度，与像素资源的 540x960 竖屏基准一致。
const float kDesignResolutionHeight = 960.0F;

/// 目标帧率。
const double kTargetFramesPerSecond = 60.0;

/// 桌面端窗口标题。
const char* const kWindowTitle = "Demon-Realm";

/// 渲染上下文每个颜色通道的位数。
const int kColorChannelBits = 8;

/// 渲染上下文深度缓冲位数。
const int kDepthBufferBits = 24;

/// 渲染上下文模板缓冲位数。
const int kStencilBufferBits = 8;

/// 多重采样数量；像素风格贴图不使用多重采样，避免边缘被抗锯齿。
const int kMultisamplingCount = 0;

/// 按配置构造单个英雄的展示信息。
/// 参数 hero：英雄配置。
/// 返回值：填充完成的英雄展示信息。
BattleHeroPresentation buildHeroPresentation(const HeroConfig& hero)
{
    BattleHeroPresentation presentation;
    presentation.heroId = hero.id;
    presentation.displayName = hero.displayName;
    presentation.description = hero.description;
    presentation.cardImageFile = hero.cardImageFile;
    presentation.firstUpgradeGoldCost = hero.firstUpgradeGoldCost;

    for (const HeroSkillConfig& skill : hero.skills)
    {
        HeroSkillPresentation skillPresentation;
        skillPresentation.skillId = skill.id;
        skillPresentation.displayName = skill.displayName;
        skillPresentation.description = skill.description;
        skillPresentation.unlockLevel = skill.unlockLevel;
        presentation.skills.push_back(skillPresentation);
    }

    return presentation;
}

/// 把技能配置映射成领域技能定义。
///
/// 触发时机与效果类型在配置里是字符串，这里翻译成枚举；未知取值属于装配错误，直接失败，
/// 避免运行出一个不会产生任何效果的技能。
///
/// 参数 config：技能配置。
/// 参数 definition：映射结果输出。
/// 返回值：触发时机、效果类型与效果参数都合法时返回 true。
bool buildSkillDefinition(const HeroSkillConfig& config, SkillDefinition& definition)
{
    definition.id = config.id;
    definition.unlockLevel = config.unlockLevel;

    if (config.trigger != kTapAttackTrigger)
    {
        cocos2d::log("[GameLauncher] unsupported skill trigger: %s", config.trigger.c_str());
        return false;
    }

    definition.trigger = SkillTrigger::TapAttack;

    if (config.effectType == kDamageEffectType)
    {
        definition.effectType = SkillEffectType::Damage;
        if (!Decimal::tryParse(config.attackMultiplier, definition.damage.attackMultiplier))
        {
            cocos2d::log("[GameLauncher] skill has invalid attackMultiplier: %s", config.id.c_str());
            return false;
        }

        return true;
    }

    if (config.effectType == kPermanentAttackGrowthEffectType)
    {
        definition.effectType = SkillEffectType::PermanentAttackGrowth;
        if (!Decimal::tryParse(config.chance, definition.attackGrowth.chance)
            || !Decimal::tryParse(config.levelProductDivisor, definition.attackGrowth.levelProductDivisor))
        {
            cocos2d::log("[GameLauncher] skill has invalid growth parameters: %s", config.id.c_str());
            return false;
        }

        if (definition.attackGrowth.levelProductDivisor.isZero())
        {
            cocos2d::log("[GameLauncher] skill divisor must be greater than zero: %s", config.id.c_str());
            return false;
        }

        return true;
    }

    cocos2d::log("[GameLauncher] unsupported skill effect type: %s", config.effectType.c_str());
    return false;
}

/// 按配置构造单个英雄的运行时状态并追加到列表。
/// 参数 hero：英雄配置。
/// 参数 heroLevel：英雄当前等级。
/// 参数 heroStates：输出列表。
/// 返回值：数值字段与技能定义合法时返回 true。
bool appendHeroState(const HeroConfig& hero, std::vector<HeroState>& heroStates)
{
    HeroSetup setup;
    setup.heroId = hero.id;
    setup.heroLevel = hero.baseHeroLevel;
    setup.attackLevel = kInitialAttackLevel;
    setup.attackIntervalLevel = kInitialAttackIntervalLevel;

    if (!Decimal::tryParse(hero.baseAttack, setup.baseAttack)
        || !Decimal::tryParse(hero.baseAttackIntervalSeconds, setup.baseAttackIntervalSeconds)
        || !Decimal::tryParse(hero.attackUpgradeBaseGain, setup.attackUpgradeBaseGain)
        || !Decimal::tryParse(hero.firstUpgradeGoldCost, setup.firstUpgradeGoldCost)
        || !Decimal::tryParse(hero.upgradeCostMultiplier, setup.upgradeCostMultiplier))
    {
        cocos2d::log("[GameLauncher] hero has invalid numeric config: %s", hero.id.c_str());
        return false;
    }

    if (setup.baseAttackIntervalSeconds.isZero())
    {
        cocos2d::log("[GameLauncher] hero attack interval must be greater than zero: %s", hero.id.c_str());
        return false;
    }

    for (const HeroAttackLevelMultiplierRange& rangeConfig : hero.attackLevelMultiplierRanges)
    {
        AttackLevelMultiplierRange range;
        range.minLevel = rangeConfig.minLevel;
        range.maxLevel = rangeConfig.maxLevel;
        if (!Decimal::tryParse(rangeConfig.multiplier, range.multiplier))
        {
            cocos2d::log("[GameLauncher] hero has invalid attack level multiplier: %s", hero.id.c_str());
            return false;
        }

        setup.attackLevelMultiplierRanges.push_back(range);
    }

    for (const HeroSkillConfig& skillConfig : hero.skills)
    {
        SkillDefinition definition;
        if (!buildSkillDefinition(skillConfig, definition))
        {
            return false;
        }

        setup.skills.push_back(definition);
    }

    heroStates.emplace_back(std::move(setup));
    return true;
}

/// 按配置装配战斗用例。
/// 参数 boss：当前 Boss 配置。
/// 参数 heroConfigs：参与战斗的英雄配置列表，顺序即结算与展示顺序。
/// 参数 heroLevel：英雄当前等级。
/// 返回值：装配成功返回战斗用例；配置数值非法时返回 nullptr。
std::unique_ptr<BattleController> createBattleController(const BossConfig& boss,
                                                        const std::vector<HeroConfig>& heroConfigs)
{
    Decimal bossMaxHp;
    if (!Decimal::tryParse(boss.maxHp, bossMaxHp))
    {
        cocos2d::log("[GameLauncher] boss has invalid maxHp: %s", boss.id.c_str());
        return nullptr;
    }

    BattleScenePresentation presentation;
    presentation.backgroundImageFile = boss.backgroundImageFile;
    presentation.bossImageFile = boss.idleImageFile;

    std::vector<HeroState> heroStates;
    heroStates.reserve(heroConfigs.size());
    for (const HeroConfig& hero : heroConfigs)
    {
        if (!appendHeroState(hero, heroStates))
        {
            return nullptr;
        }

        presentation.heroes.push_back(buildHeroPresentation(hero));
    }

    // 世界状态由用例持有，战斗与升级两个系统共享同一份状态的引用。
    std::unique_ptr<GameWorld> world(new GameWorld(bossMaxHp, std::move(heroStates)));

    // 概率技能按当前时间取随机数，因此没有保底；随机源由战斗系统持有。
    std::unique_ptr<RandomSource> randomSource(new TimeSeededRandomSource());
    std::unique_ptr<CombatSystem> combatSystem(new CombatSystem(*world, std::move(randomSource)));
    std::unique_ptr<HeroUpgradeSystem> upgradeSystem(new HeroUpgradeSystem(*world));
    return std::unique_ptr<BattleController>(new BattleController(std::move(world),
                                                                 std::move(combatSystem),
                                                                 std::move(upgradeSystem),
                                                                 std::move(presentation)));
}

/// 加载运行时配置并装配战斗用例。
/// 返回值：装配成功返回战斗用例；配置缺失或非法时返回 nullptr。
std::unique_ptr<BattleController> loadBattleController()
{
    ConfigService configService;
    if (!configService.load())
    {
        cocos2d::log("[GameLauncher] failed to load runtime config");
        return nullptr;
    }

    const std::vector<BossConfig>& bosses = configService.getBosses();
    const std::vector<HeroConfig>& heroes = configService.getHeroes();
    if (bosses.empty() || heroes.empty())
    {
        cocos2d::log("[GameLauncher] runtime config has no boss or hero entry");
        return nullptr;
    }

    // 当前只有首个 Boss 参与战斗；全部已配置英雄都视为已召唤，队伍与关卡选择由后续系统决定。
    return createBattleController(bosses.front(), heroes);
}

}  // namespace

GameLauncher::GameLauncher() = default;

GameLauncher::~GameLauncher() = default;

void GameLauncher::initGLContextAttrs()
{
    // GLContextAttrs 与 ResolutionPolicy 由引擎声明在全局作用域，不在 cocos2d 命名空间内。
    ::GLContextAttrs glContextAttrs = {kColorChannelBits,
                                       kColorChannelBits,
                                       kColorChannelBits,
                                       kColorChannelBits,
                                       kDepthBufferBits,
                                       kStencilBufferBits,
                                       kMultisamplingCount};
    cocos2d::GLView::setGLContextAttrs(glContextAttrs);
}

bool GameLauncher::applicationDidFinishLaunching()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return false;
    }

    cocos2d::GLView* glView = director->getOpenGLView();
    if (glView == nullptr)
    {
        glView = cocos2d::GLViewImpl::createWithRect(
            kWindowTitle,
            cocos2d::Rect(0.0F, 0.0F, kDesignResolutionWidth, kDesignResolutionHeight));
        if (glView == nullptr)
        {
            cocos2d::log("[GameLauncher] failed to create render view");
            return false;
        }

        director->setOpenGLView(glView);
    }

    // 设计分辨率与像素资源基准一致，窗口按 1:1 呈现，避免非整数缩放破坏像素格。
    glView->setDesignResolutionSize(kDesignResolutionWidth,
                                    kDesignResolutionHeight,
                                    ::ResolutionPolicy::SHOW_ALL);
    director->setAnimationInterval(1.0 / kTargetFramesPerSecond);

    _battleController = loadBattleController();
    if (_battleController == nullptr)
    {
        return false;
    }

    // 场景只拿到非拥有指针：业务状态由组合根持有，页面切换或场景重建都不会丢失战斗进度。
    MainSceneView* scene = MainSceneView::create(_battleController.get());
    if (scene == nullptr)
    {
        cocos2d::log("[GameLauncher] failed to create main scene");
        return false;
    }

    director->runWithScene(scene);
    return true;
}

void GameLauncher::applicationDidEnterBackground()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return;
    }

    director->stopAnimation();
}

void GameLauncher::applicationWillEnterForeground()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return;
    }

    director->startAnimation();
}

}  // namespace DemonRealm
