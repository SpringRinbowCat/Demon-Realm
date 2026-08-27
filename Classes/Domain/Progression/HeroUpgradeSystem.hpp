#pragma once

#include <cstddef>

#include "Domain/Numeric/Decimal.hpp"
#include "Domain/World/GameWorld.hpp"

namespace DemonRealm
{

/// 一次升级的预览信息。
///
/// 供界面展示"升一级会得到什么、要花多少钱、现在买不买得起"，不改变任何状态。
struct HeroUpgradePreview
{
    /// 升级带来的变化量：攻击力升级是攻击力增量，攻击速度升级是缩短的秒数。
    Decimal delta;

    /// 升级所需金币。
    Decimal goldCost;

    /// 当前金币是否足够。
    bool affordable = false;

    /// 升级是否会产生实际效果。
    ///
    /// 攻击间隔已经小到 1% 不足最小精度时，升级不会缩短任何时间，此时不该让玩家白花钱。
    bool effective = false;
};

/// 一次升级的结果。
struct HeroUpgradeOutcome
{
    /// 升级是否成功执行。
    bool applied = false;

    /// 实际花掉的金币；未执行时为 0。
    Decimal spentGold;
};

/// 英雄升级系统。
///
/// 职责：按既定公式推进英雄的攻击力与攻击速度成长，并从经济状态里扣除费用。所有升级都
/// 必须经过本类，界面不允许直接改英雄状态或金币。
///
/// 升级公式：
/// - 攻击力：1 级升 2 级加配置的基础增量，之后每级的增量按当前攻击力等级所在区间的倍率累乘。
/// - 攻击速度：每次缩短当前基础攻击间隔的 1%，因此缩短量随间隔变小而递减，间隔不会到 0。
/// - 英雄等级：攻击力或攻击速度任一项升级都加一，技能解锁按英雄等级判定。
/// - 费用：攻击力与攻击速度共用一条费用序列，只随英雄等级按配置的倍率递增。
///
/// 金币不足或升级不会产生效果时一律拒绝，不扣钱也不改状态。
///
/// 线程要求：非线程安全，只在主线程使用。
class HeroUpgradeSystem
{
public:
    /// 构造升级系统。
    /// 参数 world：世界状态，本类只引用不拥有，必须比本类活得更久。
    explicit HeroUpgradeSystem(GameWorld& world);

    /// 预览攻击力升级。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：预览信息；序号越界时各项为默认值。
    HeroUpgradePreview previewAttackUpgrade(std::size_t heroIndex) const;

    /// 预览攻击速度升级。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：预览信息；序号越界时各项为默认值。
    HeroUpgradePreview previewAttackIntervalUpgrade(std::size_t heroIndex) const;

    /// 升级攻击力。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：升级结果；金币不足或序号越界时不执行。
    HeroUpgradeOutcome upgradeAttack(std::size_t heroIndex);

    /// 升级攻击速度。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：升级结果；金币不足、序号越界或缩短量为 0 时不执行。
    HeroUpgradeOutcome upgradeAttackInterval(std::size_t heroIndex);

private:
    /// 取指定序号的英雄状态。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：越界时返回 nullptr。
    HeroState* _findHero(std::size_t heroIndex);

    /// 取指定序号的英雄状态的只读指针。
    /// 参数 heroIndex：英雄序号。
    /// 返回值：越界时返回 nullptr。
    const HeroState* _findHero(std::size_t heroIndex) const;

    /// 世界状态，非拥有引用。
    GameWorld& _world;
};

}  // namespace DemonRealm
