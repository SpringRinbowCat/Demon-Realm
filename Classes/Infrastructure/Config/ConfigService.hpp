#pragma once

#include <vector>

#include "Infrastructure/Config/ConfigTypes.hpp"

namespace DemonRealm
{

/// 运行时配置加载服务。
///
/// 职责：从 `Resources/Config/` 读取 Boss 与英雄配置，校验版本和字段后转换为
/// 内存中的配置对象。该服务只做加载、解析和校验，不实现战斗、经济或升级规则，
/// 也不持有游戏状态。
///
/// 使用场景：由组合根在启动阶段调用一次，随后把配置对象交给业务层或视图数据使用。
///
/// 线程要求：当前实现为同步读取，只能在主线程调用。
class ConfigService
{
public:
    /// 加载并校验全部运行时配置。
    /// 返回值：全部配置加载成功返回 true；文件缺失、解析失败或字段非法返回 false，
    /// 此时已加载的内容会被清空，避免留下半更新状态。
    bool load();

    /// 返回 Boss 配置列表，顺序与配置文件一致。
    const std::vector<BossConfig>& getBosses() const;

    /// 返回英雄配置列表，顺序与配置文件一致。
    const std::vector<HeroConfig>& getHeroes() const;

private:
    /// 加载并校验 Boss 配置。
    /// 返回值：成功返回 true。
    bool _loadBosses();

    /// 加载并校验英雄配置。
    /// 返回值：成功返回 true。
    bool _loadHeroes();

    /// 已加载的 Boss 配置。
    std::vector<BossConfig> _bosses;

    /// 已加载的英雄配置。
    std::vector<HeroConfig> _heroes;
};

}  // namespace DemonRealm
