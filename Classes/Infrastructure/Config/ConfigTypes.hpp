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
struct HeroSkillConfig
{
    /// 技能 id。
    std::string id;

    /// 技能展示名称。
    std::string displayName;

    /// 解锁所需的英雄等级。
    int unlockLevel = 0;
};

/// 英雄配置数据。
struct HeroConfig
{
    /// 配置 id，对应 heroes.json 的 `id`。
    std::string id;

    /// 展示名称。
    std::string displayName;

    /// 基础攻击力，十进制字符串，例如 "1"。
    std::string baseAttack;

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
