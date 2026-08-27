#pragma once

#include <string>
#include <vector>

namespace DemonRealm
{

/// 英雄技能的展示信息。
///
/// 只保存展示文案：技能的解锁等级与效果由领域层的技能定义描述，解锁判定也在领域层完成，
/// 避免同一条规则在两处各写一份。
struct HeroSkillPresentation
{
    /// 技能 id，用于与领域层的技能定义对齐。
    std::string skillId;

    /// 技能展示名称。
    std::string displayName;
};

/// 单个英雄与战斗无关的展示信息。
///
/// 这些字段不会随战斗推进变化，由组合根按配置填充一次，避免每帧重复拷贝。
struct BattleHeroPresentation
{
    /// 英雄配置 id，用于与运行时状态对齐。
    std::string heroId;

    /// 英雄展示名称。
    std::string displayName;

    /// 卡片立绘文件名，不含目录。
    std::string cardImageFile;

    /// 技能列表，按配置顺序保留；解锁判定按当前等级在产出快照时完成。
    std::vector<HeroSkillPresentation> skills;
};

/// 战斗场景与战斗推进无关的展示信息。
struct BattleScenePresentation
{
    /// 战斗背景图文件名，不含目录。
    std::string backgroundImageFile;

    /// Boss 贴图文件名，不含目录。
    std::string bossImageFile;

    /// 英雄展示信息，顺序需与战斗系统内的英雄顺序一致。
    std::vector<BattleHeroPresentation> heroes;
};

}  // namespace DemonRealm
