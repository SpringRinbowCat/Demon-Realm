#pragma once

#include <functional>
#include <string>

#include "cocos2d.h"

#include "Application/BattleSnapshot.hpp"

namespace DemonRealm
{

/// 英雄详情里的升级入口类型。
enum class HeroUpgradeKind
{
    /// 升级攻击力。
    Attack,

    /// 升级攻击速度，即缩短攻击间隔。
    AttackInterval
};

/// 单个英雄的卡片视图，支持点击展开详情。
///
/// 职责：铺设一张英雄卡片的折叠信息与展开详情，并把"请求展开/收起"和"请求升级"两类
/// 输入回传给上层。视图只做表现与输入回传，不判断能否升级、也不计算升级后的数值。
///
/// 坐标约定：卡片内容从自身原点向下生长，即原点是卡片左上角，子节点的 y 都不大于 0。
/// 展开与收起只改变 `getPreferredHeight()`，不需要重新定位卡片内部的任何节点，
/// 上层据此重排列表即可。
///
/// 线程要求：只能在主线程创建、访问和销毁。
class HeroCardView : public cocos2d::Node
{
public:
    /// 展开或收起请求的回调类型。
    using ToggleCallback = std::function<void()>;

    /// 升级请求的回调类型；参数为被点击的升级入口。
    using UpgradeCallback = std::function<void(HeroUpgradeKind)>;

    /// 折叠状态下的卡片高度，与 面板_英雄卡片.png 的高度一致。
    static const float kFoldedHeight;

    /// 创建英雄卡片视图。
    /// 参数 hero：英雄快照。
    /// 返回值：已加入自动释放池的视图对象；分配或资源缺失时返回 nullptr。
    static HeroCardView* create(const BattleHeroSnapshot& hero);

    /// 初始化卡片内容。
    /// 参数 hero：英雄快照。
    /// 返回值：初始化成功返回 true。
    bool initWithHero(const BattleHeroSnapshot& hero);

    /// 刷新随战斗变化的数值文字。
    ///
    /// 只改文字不重建节点，因此不会打断列表滚动，也不会改变卡片高度。
    ///
    /// 参数 hero：最新的英雄快照。
    void updateHero(const BattleHeroSnapshot& hero);

    /// 展开或收起详情。
    /// 参数 expanded：true 表示展开。
    void setExpanded(bool expanded);

    /// 详情是否处于展开状态。
    bool isExpanded() const;

    /// 当前应占用的高度：折叠时是卡片高度，展开时含详情高度。
    float getPreferredHeight() const;

    /// 设置展开或收起请求的回调。
    /// 参数 callback：上层注入的回调。
    void setOnToggleRequested(const ToggleCallback& callback);

    /// 设置升级请求的回调。
    /// 参数 callback：上层注入的回调。
    void setOnUpgradeRequested(const UpgradeCallback& callback);

private:
    /// 铺设折叠区：面板、立绘与四行信息文字。
    /// 参数 hero：英雄快照。
    /// 返回值：成功返回 true。
    bool _setUpFoldedArea(const BattleHeroSnapshot& hero);

    /// 铺设展开区：两个升级块、英雄介绍与技能介绍。
    /// 参数 hero：英雄快照。
    /// 返回值：成功返回 true。
    bool _setUpDetailArea(const BattleHeroSnapshot& hero);

    /// 按最新快照重建展开区，并保持当前的展开状态。
    ///
    /// 升级会改变等级、增量、花费与技能说明里的数值，展开区高度也可能变化，因此整块重建。
    void _rebuildDetailArea();

    /// 一个升级块的展示文字。
    ///
    /// 数值的格式化在调用处完成：攻击力按整数展示、攻击间隔按秒展示，两者规则不同，
    /// 因此这里只接收最终文字。
    struct UpgradeBlockTexts
    {
        /// 升级按钮文字。
        std::string buttonTitle;

        /// 等级文字，例如 "等级：1"。
        std::string level;

        /// 变化量文字，例如 "攻击力 +2" 或 "攻击间隔 待定"。
        std::string delta;

        /// 花费文字，例如 "20"。
        std::string cost;

        /// 当前金币是否够这次升级；不够时花费用红色显示。
        bool affordable = false;
    };

    /// 在展开区里铺设一个升级块：第一排是按钮、等级与变化量，第二排是花费。
    /// 参数 texts：块内的展示文字。
    /// 参数 kind：升级入口类型，用于回传。
    /// 参数 cursorY：展开区内的排版游标，向下生长，会被更新。
    /// 返回值：成功返回 true。
    bool _setUpUpgradeBlock(const UpgradeBlockTexts& texts, HeroUpgradeKind kind, float& cursorY);

    /// 在展开区里铺设英雄介绍与技能介绍。
    /// 参数 hero：英雄快照。
    /// 参数 cursorY：展开区内的排版游标，向下生长，会被更新。
    /// 返回值：成功返回 true。
    bool _setUpDescriptions(const BattleHeroSnapshot& hero, float& cursorY);

    /// 注册折叠区的点击输入。
    /// 返回值：成功返回 true。
    bool _setUpToggleInput();

    /// 判断触点是否落在折叠区内。
    ///
    /// 展开区里的升级按钮自己处理点击，因此命中区域只取折叠区，避免点按钮时顺带收起详情。
    ///
    /// 参数 worldPoint：世界坐标下的触点。
    /// 返回值：落在折叠区内返回 true。
    bool _isInsideFoldedArea(const cocos2d::Vec2& worldPoint) const;

    /// 英雄快照副本，用于刷新时比对上一次的数值。
    BattleHeroSnapshot _hero;

    /// 展开区容器；折叠时隐藏。生命周期由节点树持有。
    cocos2d::Node* _detailNode = nullptr;

    /// 展开区高度。
    float _detailHeight = 0.0F;

    /// 详情是否展开。
    bool _expanded = false;

    /// 等级文字。
    cocos2d::Label* _levelLabel = nullptr;

    /// 攻击力文字。
    cocos2d::Label* _attackLabel = nullptr;

    /// 攻击间隔文字。
    cocos2d::Label* _attackIntervalLabel = nullptr;

    /// 已解锁技能文字。
    cocos2d::Label* _skillsLabel = nullptr;

    /// 展开或收起请求回调。
    ToggleCallback _onToggleRequested;

    /// 升级请求回调。
    UpgradeCallback _onUpgradeRequested;
};

}  // namespace DemonRealm
