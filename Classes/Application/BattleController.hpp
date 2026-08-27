#pragma once

#include <memory>

#include "Application/BattlePresentationData.hpp"
#include "Application/BattleSnapshot.hpp"
#include "Domain/Combat/CombatSystem.hpp"

namespace DemonRealm
{

/// 战斗页面的用例入口。
///
/// 职责：把外部输入（当前只有时间推进）翻译成对战斗系统的调用，并把业务状态整理成
/// 展示快照。表现层只依赖本类，不直接接触 Domain 状态；后续的点击攻击、技能释放、
/// 英雄升级都在这里新增方法，视图不需要知道它们如何落到状态上。
///
/// 性能：`advance` 在没有任何数值变化时返回 false，视图据此跳过刷新；只有变化的那一帧
/// 才会调用 `createStatusSnapshot` 生成两个字符串。英雄卡片这类静态信息只在建立界面时
/// 通过 `createSnapshot` 产出一次。
///
/// 线程要求：非线程安全，只在主线程使用。
class BattleController
{
public:
    /// 构造战斗用例。
    /// 参数 combatSystem：战斗系统，不能为空，由本类持有。
    /// 参数 presentation：与战斗推进无关的展示信息，英雄顺序需与战斗系统一致。
    BattleController(std::unique_ptr<CombatSystem> combatSystem, BattleScenePresentation presentation);

    /// 推进战斗。
    /// 参数 deltaSeconds：距上次推进的秒数。
    /// 返回值：展示数值发生变化返回 true，调用方据此决定是否刷新界面。
    bool advance(double deltaSeconds);

    /// 产出完整快照，用于首次建立界面或整屏重建。
    BattleSnapshot createSnapshot() const;

    /// 产出数值快照，用于每帧刷新金币与血量。
    BattleStatusSnapshot createStatusSnapshot() const;

    /// Boss 是否已被击败。
    bool isBossDefeated() const;

private:
    /// 战斗系统。
    std::unique_ptr<CombatSystem> _combatSystem;

    /// 与战斗推进无关的展示信息。
    BattleScenePresentation _presentation;
};

}  // namespace DemonRealm
