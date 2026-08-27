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

/// 单个英雄的展示快照。
struct BattleHeroSnapshot
{
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
