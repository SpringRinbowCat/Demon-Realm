#include "Presentation/MainSceneView.hpp"

#include <new>

#include "cocos2d.h"

#include "Presentation/EnterGameView.hpp"

namespace DemonRealm
{
namespace
{

/// 页面的渲染层级；同一时间只挂载一个页面。
const int kPageZOrder = 0;

/// 页面切换调度键与延迟。
///
/// 页面切换必须延迟到下一帧：点击回调是在触摸分发过程中执行的，若此时直接移除
/// 当前页面，会在分发未结束时销毁正在处理触摸的按钮节点。
const char* const kPageSwitchScheduleKey = "page_switch";
const float kPageSwitchDelaySeconds = 0.0F;

}  // namespace

MainSceneView* MainSceneView::create(BattleController* battleController)
{
    MainSceneView* scene = new (std::nothrow) MainSceneView();
    if (scene == nullptr)
    {
        cocos2d::log("[MainSceneView] failed to allocate scene");
        return nullptr;
    }

    if (!scene->initWithBattleController(battleController))
    {
        // 初始化失败的对象还没有进入自动释放池，这里直接释放，避免泄漏。
        delete scene;
        return nullptr;
    }

    scene->autorelease();
    return scene;
}

bool MainSceneView::initWithBattleController(BattleController* battleController)
{
    if (!cocos2d::Scene::init())
    {
        return false;
    }

    if (battleController == nullptr)
    {
        cocos2d::log("[MainSceneView] battle controller is required");
        return false;
    }

    _battleController = battleController;
    return _setUpEnterGamePage();
}

void MainSceneView::update(float delta)
{
    if (_battleController == nullptr || _battleView == nullptr)
    {
        return;
    }

    _applyRefreshRequest(_battleController->advance(static_cast<double>(delta)));

    if (_battleController->isBossDefeated())
    {
        // 关卡推进尚未实现：Boss 被击败后停止推进，避免每帧空转。
        cocos2d::log("[MainSceneView] boss defeated; stage progression is not implemented yet");
        unscheduleUpdate();
    }
}

bool MainSceneView::_setUpEnterGamePage()
{
    EnterGameView* enterGameView = EnterGameView::create();
    if (enterGameView == nullptr)
    {
        cocos2d::log("[MainSceneView] failed to create enter game page");
        return false;
    }

    enterGameView->setOnEnterGameRequested([this]() { _onEnterGameRequested(); });
    addChild(enterGameView, kPageZOrder);
    _enterGameView = enterGameView;
    return true;
}

void MainSceneView::_onEnterGameRequested()
{
    cocos2d::log("[MainSceneView] enter game requested; switching to battle page");
    scheduleOnce([this](float) { _showBattlePage(); },
                 kPageSwitchDelaySeconds,
                 kPageSwitchScheduleKey);
}

void MainSceneView::_showBattlePage()
{
    BattleView* battleView = BattleView::create(_battleController->createSnapshot());
    if (battleView == nullptr)
    {
        cocos2d::log("[MainSceneView] failed to create battle page");
        return;
    }

    battleView->setOnBottomBarItemSelected(
        [this](BattleBottomBarItem item) { _onBottomBarItemSelected(item); });
    battleView->setOnBossTapped([this]() { _onBossTapped(); });
    battleView->setOnHeroUpgradeRequested(
        [this](std::size_t heroIndex, HeroUpgradeKind kind) { _onHeroUpgradeRequested(heroIndex, kind); });

    if (_enterGameView != nullptr)
    {
        removeChild(_enterGameView);
        _enterGameView = nullptr;
    }

    addChild(battleView, kPageZOrder);
    _battleView = battleView;

    // 进入战斗页后才开始推进战斗，进入游戏页停留期间不产生伤害与金币。
    scheduleUpdate();
}

void MainSceneView::_onBossTapped()
{
    if (_battleController == nullptr)
    {
        return;
    }

    _applyRefreshRequest(_battleController->onBossTapped());
}

void MainSceneView::_onHeroUpgradeRequested(std::size_t heroIndex, HeroUpgradeKind kind)
{
    if (_battleController == nullptr)
    {
        return;
    }

    // 能否升级由用例判断：金币不足或升级已无效果时返回空的刷新范围，界面保持原样。
    const BattleController::RefreshRequest request = kind == HeroUpgradeKind::Attack
        ? _battleController->onAttackUpgradeRequested(heroIndex)
        : _battleController->onAttackIntervalUpgradeRequested(heroIndex);
    _applyRefreshRequest(request);
}

void MainSceneView::_applyRefreshRequest(const BattleController::RefreshRequest& request)
{
    if (_battleView == nullptr || !request.hasAny())
    {
        return;
    }

    if (request.status)
    {
        _battleView->updateStatus(_battleController->createStatusSnapshot());
    }

    if (request.heroes)
    {
        _battleView->updateHeroes(_battleController->createHeroSnapshots());
    }
}

void MainSceneView::_onBottomBarItemSelected(BattleBottomBarItem item)
{
    // 页面切换属于场景宿主的职责；除战斗页面外的目标页面尚未实现。
    if (item == BattleBottomBarItem::Battle)
    {
        cocos2d::log("[MainSceneView] already on the battle page");
        return;
    }

    cocos2d::log("[MainSceneView] target page is not implemented yet: %d", static_cast<int>(item));
}

}  // namespace DemonRealm
