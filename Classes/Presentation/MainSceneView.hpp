#pragma once

#include "cocos2d.h"

#include "Application/BattleController.hpp"
#include "Presentation/BattleView.hpp"

namespace DemonRealm
{

class EnterGameView;

/// 主场景视图，单场景宿主的表现根节点。
///
/// 职责：作为唯一场景的根节点挂载当前页面、承担页面之间的切换，并把引擎的每帧时间
/// 推进转发给战斗用例。场景不持有业务状态，也不计算业务规则；伤害与金币结算全部发生在
/// `BattleController` 之后的业务层。
///
/// 时间推进：进入战斗页后才开始每帧推进，Boss 被击败后停止推进，避免空转。挂机离线收益
/// 需要独立的时间源与补算逻辑，不依赖这里的帧驱动。
///
/// 线程要求：只能在主线程创建、访问和销毁。
class MainSceneView : public cocos2d::Scene
{
public:
    /// 创建主场景。
    /// 参数 battleController：战斗用例，非拥有指针，必须在场景之前创建、之后销毁，
    ///     由组合根持有；传入 nullptr 时战斗页面无法建立。
    /// 返回值：已加入自动释放池的场景对象；分配或初始化失败时返回 nullptr。
    static MainSceneView* create(BattleController* battleController);

    /// 初始化场景内容，挂载进入游戏页面。
    /// 参数 battleController：战斗用例，非拥有指针。
    /// 返回值：初始化成功返回 true；基类初始化或页面创建失败返回 false。
    bool initWithBattleController(BattleController* battleController);

    /// 每帧推进战斗，并在数值变化时刷新战斗页面。
    /// 参数 delta：距上一帧的秒数，由引擎调度器传入。
    void update(float delta) override;

private:
    /// 挂载进入游戏页面，并接入页面回传的进入游戏请求。
    /// 返回值：页面创建并挂载成功返回 true。
    bool _setUpEnterGamePage();

    /// 进入游戏请求响应；把页面切换安排到下一帧执行。
    void _onEnterGameRequested();

    /// 用战斗页面替换当前页面，并开始每帧推进战斗。
    void _showBattlePage();

    /// 底部栏入口响应；当前只有战斗页面已实现，其余入口只输出日志。
    /// 参数 item：被点击的入口项。
    void _onBottomBarItemSelected(BattleBottomBarItem item);

    /// Boss 点击响应；转交战斗用例结算点击类技能。
    void _onBossTapped();

    /// 按刷新范围更新战斗页面。
    /// 参数 request：需要刷新的界面范围。
    void _applyRefreshRequest(const BattleController::RefreshRequest& request);

    /// 战斗用例；由组合根持有，这里只保存非拥有指针。
    BattleController* _battleController = nullptr;

    /// 进入游戏页面视图；生命周期由节点树持有，这里只保存非拥有引用。
    EnterGameView* _enterGameView = nullptr;

    /// 战斗页面视图；生命周期由节点树持有，这里只保存非拥有引用。
    BattleView* _battleView = nullptr;
};

}  // namespace DemonRealm
