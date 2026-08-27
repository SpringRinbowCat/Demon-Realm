#pragma once

#include "Domain/Modifier/ModifierAggregate.hpp"
#include "Domain/Modifier/ModifierCollection.hpp"
#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{

/// 金币等经济数值的运行时状态。
///
/// 职责：保存金币余额，并按"伤害量 + 金币产出修正"结算收益。当前规则是每造成 1 点伤害
/// 获得 1 金币，金币加成通过 GoldGain 修正表达，不在这里写死系数。
///
/// 线程要求：非线程安全，只在推进战斗的线程使用。
class EconomyState
{
public:
    EconomyState();

    /// 按伤害量结算金币收益并入账。
    /// 参数 damage：实际造成的伤害量。
    /// 参数 globalGoldAggregate：全局金币产出修正聚合。
    /// 返回值：本次实际获得的金币，已计入自身与全局的金币产出修正。
    Decimal addGoldFromDamage(const Decimal& damage, const ModifierAggregate& globalGoldAggregate);

    /// 直接增加金币，用于奖励、离线收益等已经算好的收益。
    /// 参数 amount：增加量。
    void addGold(const Decimal& amount);

    /// 扣除金币，余额不足时不做任何改动。
    /// 参数 amount：扣除量。
    /// 返回值：余额足够并完成扣除返回 true。
    bool trySpendGold(const Decimal& amount);

    /// 取当前金币余额。
    const Decimal& getGoldAmount() const;

    /// 取自身的金币修正集合，供活动、装备等系统增删 buff。
    ModifierCollection& getModifiers();

    /// 取自身的金币修正集合的只读引用。
    const ModifierCollection& getModifiers() const;

private:
    /// 金币余额。
    Decimal _goldAmount;

    /// 作用于本状态的修正集合，目前只使用 GoldGain 目标。
    ModifierCollection _modifiers;
};

}  // namespace DemonRealm
