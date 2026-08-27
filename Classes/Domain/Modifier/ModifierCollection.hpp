#pragma once

#include <array>
#include <string>
#include <vector>

#include "Domain/Modifier/ModifierAggregate.hpp"
#include "Domain/Modifier/ModifierTypes.hpp"

namespace DemonRealm
{

/// 一组属性修正的容器，按目标提供聚合结果。
///
/// 职责：收集来自技能、装备、活动等来源的修正，并按目标给出聚合结果。聚合结果带缓存，
/// 只有在修正集合发生变化后的首次查询才重算，因此可以在每帧的战斗结算中放心查询。
///
/// 版本号：每次增删修正都会自增 `revision`，外部（例如英雄的派生属性缓存）可以据此
/// 判断"是否需要重算依赖本集合的结果"，避免每帧重复计算大数运算。
///
/// 线程要求：非线程安全；由持有它的状态对象在主线程使用。
class ModifierCollection
{
public:
    ModifierCollection();

    /// 添加一条修正。
    /// 参数 modifier：待添加的修正。
    void add(const Modifier& modifier);

    /// 按来源移除全部修正。
    /// 参数 sourceId：来源 id。
    /// 返回值：确实移除了至少一条返回 true。
    bool removeBySource(const std::string& sourceId);

    /// 清空全部修正。
    void clear();

    /// 取某个目标的聚合结果。
    /// 参数 target：修正目标。
    /// 返回值：聚合结果的常引用，生命周期到下一次增删修正为止。
    const ModifierAggregate& getAggregate(ModifierTarget target) const;

    /// 取版本号，供外部做缓存失效判断。
    /// 返回值：从 0 开始，每次增删修正自增。
    unsigned long long getRevision() const;

private:
    /// 重算全部目标的聚合结果。
    void _rebuildAggregates() const;

    /// 全部修正，按添加顺序保存。
    std::vector<Modifier> _modifiers;

    /// 按目标索引的聚合缓存。
    mutable std::array<ModifierAggregate, kModifierTargetCount> _aggregates;

    /// 聚合缓存是否有效。
    mutable bool _aggregatesValid;

    /// 修正集合版本号。
    unsigned long long _revision;
};

}  // namespace DemonRealm
