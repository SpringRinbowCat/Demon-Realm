#pragma once

#include <string>
#include <vector>

namespace DemonRealm
{

/// Boss 配置数据。
///
/// 只包含当前已被使用的字段；掉落等经济相关字段等对应系统落地后再补充解析，
/// 避免出现没有读取方的死数据。
///
/// 数值字段一律以字符串原样保留：配置里的数值会膨胀到浮点无法精确表示的量级，
/// 解析阶段只校验格式，转换成定点数值由业务层完成。
struct BossConfig
{
    /// 配置 id，对应 bosses.json 的 `id`。
    std::string id;

    /// 展示名称。
    std::string displayName;

    /// 初始最大血量，十进制字符串，例如 "100000"。
    std::string maxHp;

    /// 战斗背景图文件名，不含目录。
    std::string backgroundImageFile;

    /// 待机贴图文件名，不含目录。
    std::string idleImageFile;
};

/// 英雄技能配置数据。
///
/// 触发时机与效果类型以配置里的原始标识保留，映射成领域枚举由业务层完成；
/// 效果参数按 `effectType` 取用对应字段，其余字段为空。
struct HeroSkillConfig
{
    /// 技能 id。
    std::string id;

    /// 技能展示名称。
    std::string displayName;

    /// 技能说明文字，用于英雄详情里的技能介绍。
    std::string description;

    /// 解锁所需的英雄等级。
    int unlockLevel = 0;

    /// 触发时机标识，当前只支持 `tapAttack`。
    std::string trigger;

    /// 效果类型标识，当前支持 `damage` 与 `permanentAttackGrowth`。
    std::string effectType;

    /// 伤害倍率，十进制字符串；`effectType` 为 `damage` 时有效。
    std::string attackMultiplier;

    /// 触发概率，十进制字符串，取值范围 0 到 1；`effectType` 为 `permanentAttackGrowth` 时有效。
    std::string chance;

    /// 等级乘积的除数，十进制字符串；`effectType` 为 `permanentAttackGrowth` 时有效。
    std::string levelProductDivisor;
};

/// 攻击力等级的分段成长倍率。
///
/// 攻击力升级的增量按"上一次增量 × 当前攻击力等级所在区间的倍率"累乘，
/// 因此这些区间必须覆盖到玩家可能达到的等级，且相互不重叠。
struct HeroAttackLevelMultiplierRange
{
    /// 区间起始等级，含边界。
    int minLevel = 0;

    /// 区间结束等级，含边界。
    int maxLevel = 0;

    /// 该区间内每级的增量倍率，十进制字符串。
    std::string multiplier;
};

/// 英雄配置数据。
struct HeroConfig
{
    /// 配置 id，对应 heroes.json 的 `id`。
    std::string id;

    /// 展示名称。
    std::string displayName;

    /// 英雄说明文字，用于英雄详情里的介绍。
    std::string description;

    /// 初始英雄等级。
    ///
    /// 英雄等级与攻击力等级、攻击速度等级是三个独立的数：攻击力或攻击速度任一项升级，
    /// 英雄等级都会加一，技能解锁按英雄等级判定。
    int baseHeroLevel = 0;

    /// 基础攻击力，十进制字符串，例如 "1"。
    std::string baseAttack;

    /// 攻击力从 1 级升到 2 级的增量，十进制字符串。
    ///
    /// 之后每一级的增量在此基础上按 `attackLevelMultiplierRanges` 的倍率累乘。
    std::string attackUpgradeBaseGain;

    /// 攻击力等级的分段成长倍率，按配置顺序保留。
    std::vector<HeroAttackLevelMultiplierRange> attackLevelMultiplierRanges;

    /// 首次升级所需金币，十进制字符串。
    ///
    /// 攻击力与攻击速度各自维护一条费用序列，都从这个值起算。
    std::string firstUpgradeGoldCost;

    /// 每次升级后费用的增长倍率，十进制字符串。
    std::string upgradeCostMultiplier;

    /// 基础攻击间隔，单位秒，十进制字符串，例如 "4.0"；超过 4 位小数的部分会被向下取整。
    std::string baseAttackIntervalSeconds;

    /// UI 图标文件名，不含目录。
    std::string iconImageFile;

    /// 战斗页面卡片立绘文件名，不含目录。
    std::string cardImageFile;

    /// 技能列表，按配置顺序保留。
    std::vector<HeroSkillConfig> skills;
};

}  // namespace DemonRealm
