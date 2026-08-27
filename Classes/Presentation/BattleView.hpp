#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Application/BattleSnapshot.hpp"
#include "Presentation/HeroCardView.hpp"

namespace DemonRealm
{

/// 战斗页面底部栏的入口项。
enum class BattleBottomBarItem
{
    /// 战斗页面，当前页。
    Battle,

    /// 英雄页面，尚未实现。
    Heroes,

    /// 商店页面，尚未实现。
    Shop,

    /// 宝物页面，尚未实现。
    Treasures,

    /// 设置页面，尚未实现。
    Settings,
};

/// 战斗页面视图。
///
/// 职责：按传入的快照铺设战斗页面的三块区域——顶部金币栏与 Boss 区域、英雄信息区域、
/// 底部栏入口，并把底部栏的点击输入回传给上层。视图只做表现和输入回传，不计算伤害、
/// 金币、升级或解锁规则。
///
/// 刷新方式：战斗推进产生变化时，由上层调用 `updateStatus` 只更新金币与血量文字。
/// 视图会比对上一次的原始数值字符串，数值没变就不触碰 Label，避免每帧重排文本。
///
/// 线程要求：只能在主线程创建、访问和销毁。
class BattleView : public cocos2d::Node
{
public:
    /// 底部栏点击回调类型；参数为被点击的入口项。
    using BottomBarSelectionCallback = std::function<void(BattleBottomBarItem)>;

    /// Boss 点击回调类型。
    using BossTapCallback = std::function<void()>;

    /// 英雄升级请求回调类型；参数为英雄在列表中的序号与升级入口类型。
    using HeroUpgradeCallback = std::function<void(std::size_t, HeroUpgradeKind)>;

    /// 创建战斗页面视图。
    /// 参数 snapshot：页面快照，内部会保存所需数据的副本。
    /// 返回值：已加入自动释放池的视图对象；分配或初始化失败时返回 nullptr。
    static BattleView* create(const BattleSnapshot& snapshot);

    /// 初始化页面内容。
    /// 参数 snapshot：页面快照。
    /// 返回值：初始化成功返回 true；基类初始化或页面资源缺失时返回 false。
    bool initWithSnapshot(const BattleSnapshot& snapshot);

    /// 刷新金币与 Boss 剩余血量文字。
    /// 参数 status：最新的数值快照；与上次相同的字段不会触发文字更新。
    void updateStatus(const BattleStatusSnapshot& status);

    /// 刷新英雄栏的数值文字。
    ///
    /// 只更新等级、攻击力、攻击间隔和已解锁技能这几行文字，不重建卡片节点，
    /// 因此不会打断英雄栏的滚动位置。
    ///
    /// 参数 heroes：最新的英雄快照，顺序需与建立界面时一致。
    void updateHeroes(const std::vector<BattleHeroSnapshot>& heroes);

    /// 设置底部栏点击回调。
    /// 参数 callback：上层注入的回调；传入空回调表示只保留日志行为。
    void setOnBottomBarItemSelected(const BottomBarSelectionCallback& callback);

    /// 设置 Boss 点击回调。
    ///
    /// 命中区域是 Boss 贴图自身的矩形范围，背景与空白区域不触发。
    ///
    /// 参数 callback：上层注入的回调；传入空回调表示点击不做任何处理。
    void setOnBossTapped(const BossTapCallback& callback);

    /// 设置英雄升级请求回调。
    ///
    /// 视图不判断金币是否够、也不计算升级后的数值，只回传"玩家想升级哪个英雄的哪一项"。
    ///
    /// 参数 callback：上层注入的回调；传入空回调表示只保留日志行为。
    void setOnHeroUpgradeRequested(const HeroUpgradeCallback& callback);

private:
    /// 铺设战斗背景图，按可见区域居中，不做运行时非整数缩放。
    /// 返回值：背景加载并挂载成功返回 true。
    bool _setUpBackground();

    /// 铺设顶部金币栏，包含金币图标与数量文字。
    /// 返回值：成功返回 true。
    bool _setUpGoldBar();

    /// 铺设 Boss 贴图与剩余血量文字。
    /// 返回值：成功返回 true。
    bool _setUpBossArea();

    /// 铺设下半屏的英雄栏；英雄数量超过可见高度时可上下滑动。
    /// 返回值：成功返回 true。
    bool _setUpHeroList();

    /// 按当前展开状态重排英雄卡片，并更新可滚动内容的高度。
    void _relayoutHeroList();

    /// 英雄卡片的展开或收起请求响应。
    ///
    /// 同一时间只展开一张卡片：英雄栏一屏只放得下三张，多张同时展开会把列表拉得很长。
    ///
    /// 参数 heroIndex：被点击的英雄序号。
    void _onHeroCardToggled(std::size_t heroIndex);

    /// 在 Boss 贴图范围内接收点击并回传。
    /// 返回值：监听器注册成功返回 true。
    bool _setUpBossTapInput();

    /// 铺设底部栏的五个入口按钮。
    /// 返回值：成功返回 true。
    bool _setUpBottomBar();

    /// 底部栏点击响应；输出日志并把入口项回传给上层回调。
    /// 参数 item：被点击的入口项。
    void _onBottomBarItemClicked(BattleBottomBarItem item);

    /// 页面快照副本。
    BattleSnapshot _snapshot;

    /// 金币数量文字；生命周期由节点树持有，这里只保存非拥有引用。
    cocos2d::Label* _goldAmountLabel = nullptr;

    /// Boss 剩余血量文字；生命周期由节点树持有，这里只保存非拥有引用。
    cocos2d::Label* _bossRemainingHpLabel = nullptr;

    /// Boss 贴图；用于判断点击是否落在 Boss 范围内，只保存非拥有引用。
    cocos2d::Sprite* _bossSprite = nullptr;

    /// 英雄栏滚动容器；生命周期由节点树持有，这里只保存非拥有引用。
    cocos2d::ui::ScrollView* _heroList = nullptr;

    /// 英雄卡片视图，顺序与英雄快照一致；生命周期由节点树持有。
    std::vector<HeroCardView*> _heroCards;

    /// 上层注入的底部栏点击回调；未设置时点击只输出日志。
    BottomBarSelectionCallback _onBottomBarItemSelected;

    /// 上层注入的 Boss 点击回调；未设置时点击不做任何处理。
    BossTapCallback _onBossTapped;

    /// 上层注入的英雄升级请求回调；未设置时按钮点击只输出日志。
    HeroUpgradeCallback _onHeroUpgradeRequested;
};

}  // namespace DemonRealm
