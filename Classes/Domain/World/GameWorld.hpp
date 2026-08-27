#pragma once

#include <vector>

#include "Domain/Modifier/ModifierCollection.hpp"
#include "Domain/Numeric/Decimal.hpp"
#include "Domain/State/BossState.hpp"
#include "Domain/State/EconomyState.hpp"
#include "Domain/State/HeroState.hpp"

namespace DemonRealm
{

/// 一局游戏的全部运行时状态。
///
/// 职责：持有 Boss、经济、英雄和全局修正，作为各个领域系统共同操作的对象。战斗结算、
/// 升级、关卡推进这些系统都不拥有状态，只拿到本对象的引用，因此新增系统不需要把状态
/// 搬来搬去，也不会出现两份互相不同步的副本。
///
/// 本类只负责保管状态与提供访问入口，不实现任何规则；改变状态的规则一律写在对应的系统里。
///
/// 线程要求：非线程安全，只在主线程使用。
class GameWorld
{
public:
    /// 构造世界状态。
    /// 参数 bossMaxHp：当前 Boss 的最大血量。
    /// 参数 heroes：已召唤英雄的初始状态，顺序即结算与展示顺序。
    GameWorld(const Decimal& bossMaxHp, std::vector<HeroState> heroes);

    /// 取 Boss 状态。
    BossState& getBoss();

    /// 取 Boss 状态的只读引用。
    const BossState& getBoss() const;

    /// 取经济状态。
    EconomyState& getEconomy();

    /// 取经济状态的只读引用。
    const EconomyState& getEconomy() const;

    /// 取英雄状态列表。
    std::vector<HeroState>& getHeroes();

    /// 取英雄状态列表的只读引用。
    const std::vector<HeroState>& getHeroes() const;

    /// 按当前的全局修正刷新全部英雄的派生属性。
    ///
    /// 最终攻击力与最终攻击间隔是带缓存的派生值，任何改动了基础属性或修正的系统都必须在
    /// 改完之后调用一次，否则外部会读到改动之前的旧值。版本号未变化的英雄不会重复计算。
    void refreshHeroDerivedAttributes();

    /// 取作用于全体英雄与全局产出的修正集合。
    ModifierCollection& getGlobalModifiers();

    /// 取全局修正集合的只读引用。
    const ModifierCollection& getGlobalModifiers() const;

private:
    /// Boss 状态。
    BossState _boss;

    /// 经济状态。
    EconomyState _economy;

    /// 已召唤英雄状态。
    std::vector<HeroState> _heroes;

    /// 作用于全体英雄与全局产出的修正集合。
    ModifierCollection _globalModifiers;
};

}  // namespace DemonRealm
