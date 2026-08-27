#pragma once

#include <string>
#include <vector>

namespace DemonRealm
{

/// 战斗页面每帧可能变化的数值快照。
///
/// 数值都是规范化的定点小数字符串（固定 4 位小数），例如 "99999.0000"。展示用的千分位、
/// 小数位裁剪由表现层负责，业务层不产出带格式的文本。
struct BattleStatusSnapshot
{
    /// Boss 剩余血量。
    std::string bossRemainingHp;

    /// 金币余额。
    std::string goldAmount;
};

/// 英雄详情里一条技能的展示快照。
struct BattleHeroSkillSnapshot
{
    /// 技能展示名称。
    std::string displayName;

    /// 技能说明文字。
    std::string description;

    /// 解锁所需的英雄等级。
    int unlockLevel = 0;

    /// 是否已解锁。
    bool unlocked = false;
};

/// 英雄详情里一个升级条目的展示快照。
///
/// 攻击力与攻击速度共用一条费用序列，因此两个条目的 `costGoldAmount` 相同；
/// 各自的 `level` 与 `delta` 则相互独立。
struct BattleHeroUpgradeSnapshot
{
    /// 当前等级；取不到时为空。
    std::string level;

    /// 本次升级带来的变化量，规范化定点小数字符串；取不到时为空。
    std::string delta;

    /// 本次升级所需金币，规范化定点小数字符串；取不到时为空。
    std::string costGoldAmount;

    /// 当前金币是否够这次升级；不够时界面用红色提示，点击也会被拒绝。
    bool affordable = false;
};

/// 单个英雄的展示快照。
struct BattleHeroSnapshot
{
    /// 英雄配置 id，技能说明里的数值占位符按它匹配变量名。
    std::string heroId;

    /// 英雄展示名称。
    std::string displayName;

    /// 当前等级。
    int level = 0;

    /// 当前攻击力，规范化定点小数字符串。
    std::string attack;

    /// 当前攻击间隔（秒），规范化定点小数字符串。
    std::string attackIntervalSeconds;

    /// 已解锁技能的展示名称，按配置顺序。
    std::vector<std::string> unlockedSkillNames;

    /// 卡片立绘文件名，不含目录。
    std::string cardImageFile;

    /// 英雄说明文字，展示在英雄详情中部。
    std::string description;

    /// 全部技能的展示信息，按配置顺序；未解锁的技能也会列出并标注解锁等级。
    std::vector<BattleHeroSkillSnapshot> skills;

    /// 攻击力升级条目。
    BattleHeroUpgradeSnapshot attackUpgrade;

    /// 攻击间隔升级条目。
    BattleHeroUpgradeSnapshot attackIntervalUpgrade;
};

/// 战斗页面的完整快照，用于首次建立界面。
///
/// 由 `BattleController` 产出；视图只读取，不回写，也不据此推导业务规则。
struct BattleSnapshot
{
    /// 战斗背景图文件名，不含目录。
    std::string backgroundImageFile;

    /// Boss 贴图文件名，不含目录。
    std::string bossImageFile;

    /// 可变数值部分。
    BattleStatusSnapshot status;

    /// 英雄列表快照，按展示顺序排列。
    std::vector<BattleHeroSnapshot> heroes;
};

}  // namespace DemonRealm
