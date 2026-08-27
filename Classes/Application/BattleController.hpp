#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Application/BattlePresentationData.hpp"
#include "Application/BattleSnapshot.hpp"
#include "Domain/Combat/CombatSystem.hpp"
#include "Domain/Progression/HeroUpgradeSystem.hpp"
#include "Domain/World/GameWorld.hpp"

namespace DemonRealm
{

/// 战斗页面的用例入口。
///
/// 职责：把外部输入（时间推进、点击 Boss、请求升级）翻译成对领域系统的调用，并把业务
/// 状态整理成展示快照。表现层只依赖本类，不直接接触 Domain 状态；新增玩法时在这里加方法，
/// 视图不需要知道它如何落到状态上。
///
/// 所有权：本类持有世界状态与各个领域系统，销毁顺序由成员声明顺序保证——系统先析构，
/// 世界状态后析构，因此系统持有的世界引用在其生命周期内始终有效。
///
/// 性能：`advance` 在没有任何数值变化时返回空的刷新范围，视图据此跳过刷新；只有变化的
/// 那一帧才会生成快照。英雄卡片这类静态信息只在建立界面时产出一次。
///
/// 线程要求：非线程安全，只在主线程使用。
class BattleController
{
public:
    /// 一次战斗输入或推进之后需要刷新的界面范围。
    struct RefreshRequest
    {
        /// 金币或 Boss 剩余血量发生了变化。
        bool status = false;

        /// 英雄属性发生了变化，例如技能永久提升攻击力，或者玩家升了一级。
        bool heroes = false;

        /// 是否有任何变化。
        bool hasAny() const { return status || heroes; }
    };

    /// 构造战斗用例。
    /// 参数 world：世界状态，不能为空，由本类持有。
    /// 参数 combatSystem：战斗系统，不能为空，由本类持有。
    /// 参数 upgradeSystem：升级系统，不能为空，由本类持有。
    /// 参数 presentation：与战斗推进无关的展示信息，英雄顺序需与世界状态一致。
    BattleController(std::unique_ptr<GameWorld> world,
                     std::unique_ptr<CombatSystem> combatSystem,
                     std::unique_ptr<HeroUpgradeSystem> upgradeSystem,
                     BattleScenePresentation presentation);

    /// 推进战斗。
    /// 参数 deltaSeconds：距上次推进的秒数。
    /// 返回值：需要刷新的界面范围；没有任何变化时各项均为 false。
    RefreshRequest advance(double deltaSeconds);

    /// 处理玩家点击 Boss。
    ///
    /// 点击本身不造成伤害，伤害与附带效果来自已解锁的点击类技能；没有解锁任何点击技能时
    /// 点击不会产生任何变化。
    ///
    /// 返回值：需要刷新的界面范围。
    RefreshRequest onBossTapped();

    /// 处理升级攻击力的请求。
    /// 参数 heroIndex：英雄在列表中的序号。
    /// 返回值：需要刷新的界面范围；金币不足时不产生任何变化。
    RefreshRequest onAttackUpgradeRequested(std::size_t heroIndex);

    /// 处理升级攻击速度的请求。
    /// 参数 heroIndex：英雄在列表中的序号。
    /// 返回值：需要刷新的界面范围；金币不足或已经无法继续缩短时不产生任何变化。
    RefreshRequest onAttackIntervalUpgradeRequested(std::size_t heroIndex);

    /// 产出完整快照，用于首次建立界面或整屏重建。
    BattleSnapshot createSnapshot() const;

    /// 产出数值快照，用于每帧刷新金币与血量。
    BattleStatusSnapshot createStatusSnapshot() const;

    /// 产出英雄快照列表，用于英雄属性变化后刷新英雄栏。
    std::vector<BattleHeroSnapshot> createHeroSnapshots() const;

    /// Boss 是否已被击败。
    bool isBossDefeated() const;

private:
    /// 按英雄状态与展示信息构造英雄快照。
    /// 参数 heroIndex：英雄序号，用于取升级预览。
    /// 参数 presentation：英雄展示信息。
    /// 参数 heroState：英雄运行时状态。
    /// 返回值：填充完成的英雄快照。
    BattleHeroSnapshot _buildHeroSnapshot(std::size_t heroIndex,
                                          const BattleHeroPresentation& presentation,
                                          const HeroState& heroState) const;

    /// 若「买得起下一次升级」的状态发生翻转，则把英雄栏加入刷新范围。
    ///
    /// 花费文字在金币不足时是红色，所以金币跨过门槛时必须刷新一次；反过来，金币每帧都在
    /// 变化，只有跨过门槛才刷新可以避免每帧重建详情区。
    ///
    /// 参数 request：待补充的刷新范围。
    void _markAffordabilityChanges(RefreshRequest& request);

    /// 世界状态；必须声明在系统之前，保证析构时系统先于状态被销毁。
    std::unique_ptr<GameWorld> _world;

    /// 战斗系统。
    std::unique_ptr<CombatSystem> _combatSystem;

    /// 升级系统。
    std::unique_ptr<HeroUpgradeSystem> _upgradeSystem;

    /// 与战斗推进无关的展示信息。
    BattleScenePresentation _presentation;

    /// 每个英雄上一次产出快照时「买得起下一次升级」的状态，下标与世界里的英雄一致。
    std::vector<bool> _affordability;
};

}  // namespace DemonRealm
