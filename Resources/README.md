# Resources 模块

> 本文档中的路径均相对于 `Resources/` 目录；仓库根目录说明见 [../README.md](../README.md)，代码架构说明见 [../Classes/README.md](../Classes/README.md)。

## 模块范围与当前状态

`Resources/` 保存游戏运行时读取的配置、图片、动画、音频、字体和 Shader 资源。它不保存 C++ 代码，也不承载伤害、金币、升级、关卡或存档规则的实现。

当前资源目录已经建立基础分类，但仍处于第一版占位状态：实际可用配置主要是 `Config/bosses.json` 和 `Config/heroes.json`，运行时 PNG 主要是火焰背景、阿熊待机图和沛总 UI 图标，其余目录多数只有 `.gitkeep`。

资源加载、解析、缓存和释放由 [Classes/Infrastructure](../Classes/Infrastructure/) 中目标的 Infrastructure 模块负责；当前代码尚未实现，文档中的服务名称属于目标职责。

## 目录结构

以下路径均相对于当前 `Resources/` 目录：

```text
./
├── Config/                         # Boss、关卡、升级和奖励等运行时配置
│   ├── bosses.json
│   └── heroes.json
├── Textures/
│   ├── Pixel/                      # 像素格 PNG 贴图
│   │   ├── Bosses/
│   │   ├── Heroes/
│   │   ├── UI/
│   │   ├── Effects/
│   │   ├── Backgrounds/
│   │   └── Common/
│   └── Atlas/                      # Sprite Sheet、图集及描述文件
├── Animations/                     # 帧动画定义和动画配置
├── Audio/
│   ├── BGM/                        # 背景音乐
│   └── SFX/                        # 音效
├── Fonts/                          # 像素字体和 UI 字体
└── Shaders/                        # 受击、描边、发光等运行时 Shader
```

目录中的具体文件可能随功能增加；新的资源应放入职责匹配的分类，不要把运行时资源放回 `Classes/`。

## 运行时资源与 ArtSource 边界

- `Resources/` 只保存运行时需要读取或打包的资源。
- `ArtSource/` 只保存 PSD、Aseprite、原始压缩包、来源记录和其它美术编辑源文件，不参与运行时打包。
- `ArtSource/` 已由根目录 `.gitignore` 忽略；来源记录仍保留在本地，用于追溯授权和处理方式。
- `Classes/Infrastructure/PixelAssets/` 只保存资源访问、缓存和释放代码，不保存 PNG。
- 配置中的图片值是资源 ID 或文件名，不是 Markdown 链接，也不能因为 README 的位置变化而机械添加 `../`。实际路径由配置/资源适配层解析。

当前资源入口：

- [火焰背景](Textures/Pixel/Backgrounds/demon_realm_fire_background.png)：540×960，无 Alpha。
- [阿熊待机图](Textures/Pixel/Bosses/boss_1_idle.png)：540×420，RGBA；使用完整 Bear 64×33 帧按 10 倍最近邻放大。
- [沛总 UI 图标](Textures/Pixel/Heroes/hero_1_icon.png)：32×32，RGBA；使用 16×16 源图按 2 倍最近邻放大。

## Config 配置契约

目标加载边界是 `Classes/Infrastructure/Config/ConfigService`：负责加载、解析、版本校验、字段范围校验和错误处理；配置文件只描述数据，不实现战斗、升级或存档流程。

### Boss 配置

当前 [Config/bosses.json](Config/bosses.json) 为 `schemaVersion` 1，包含一个 id 为 `1`、名称为“阿熊”的 Boss，初始 `maxHp` 为 100000。

每个 `drops` 条目包含：

- `type`：只能是 `currency` 或 `treasure`。
- `itemId`：掉落物或宝物 ID。
- `quantity`：掉落数量。
- `probability`：0 到 1 之间的掉落概率。

`currency` 是本轮使用的常规货币，通关结算后清空；`treasure` 对应通关后保留的宝物、解锁状态或永久加成。Boss 的 `images` 当前包含 `background` 和 `idle` 两个文件名字段。

### 英雄配置

当前 [Config/heroes.json](Config/heroes.json) 为 `schemaVersion` 1，包含 id 为 `1` 的“沛总”。英雄对象中的 `attackLevelMultiplierRanges` 用于配置攻击力等级的分段成长倍率：

- `minLevel` 和 `maxLevel` 都包含边界。
- `multiplier` 作用于该区间内攻击力等级的成长。
- 区间不能重叠，且 `minLevel` 不得大于 `maxLevel`。
- 英雄等级和攻击力等级是两个独立概念。

当前配置为攻击力等级 1–30 倍率 2.0、31–80 倍率 1.5。`upgradeCostMultiplier` 只表示升级费用增长，不表示攻击力成长倍率。

## 像素图片资源规范

为避免 Boss、人物和背景的像素密度混用，运行时像素资源按显示层分档：

- **全屏战斗层**：背景、Boss、场景内人物和敌人统一使用 10×10 输出像素格。540×960 背景对应 54×96 个逻辑格；Boss 使用完整 64×33 原始帧按 10 倍最近邻放大，放入 540×420 RGBA 画布。需要叠加到战斗背景上的人物，也必须按同一 10 倍像素密度导出。
- **UI 小图标**：卡片内人物图标允许使用 16×16 源图按 2 倍最近邻放大到 32×32；它只属于 UI 层，不直接叠加在战斗背景上。同一人物如果进入战斗场景，必须另做符合 10 倍战斗像素密度的版本。
- **缩放与导出**：只能使用整数倍缩放和最近邻采样；禁止双线性、双三次、抗锯齿和运行时非整数缩放。源图尺寸、裁剪边界和场景摆放位置应对齐对应像素格。
- **透明与边缘**：Boss、人物和特效使用 RGBA；轮廓外保持透明，不留黑边、白边或原素材背景色的 matte 光晕。背景使用完整画布，不把人物背景色烘焙进背景图。
- **色阶与邻接像素**：同一背景区域使用有限调色板；相邻像素格只有在火焰轮廓、角色轮廓等有意边界处才允许出现明显色差，不用随机渐变制造细碎跳色。火焰带内部的近黑填充统一使用与上方暗红衔接的 RGB(70,6,12)，下半遮挡区 y≥400 固定为 RGB(145,14,16)。
- **背景构图**：顶部 0–112px 为金币栏等系统 UI 的暗色安全区；主要火光和 Boss 对比区域位于上半屏；y≥400 的下半区为单一遮挡色，不放置必须被看清的火光、角色轮廓或关键特效。
- **验收方式**：必须在目标显示尺寸下检查背景、Boss 和人物是否拥有相同的像素块观感；同时检查 PNG 尺寸、Alpha、缩放方式和来源授权记录。

## 资源来源与授权

来源记录按照运行时资源目录分层维护。每个运行时目录只记录该目录中资源的来源、授权和对应的 `ArtSource/` 记录，避免不同资源类型混在一起。

### `Textures/Pixel/Backgrounds/`

- [demon_realm_fire_background.png](Textures/Pixel/Backgrounds/demon_realm_fire_background.png)：项目内原创/处理资源，当前没有第三方来源记录。

### `Textures/Pixel/Bosses/`

- [boss_1_idle.png](Textures/Pixel/Bosses/boss_1_idle.png)：使用 OpenGameArt.org 的 `Animated Wild Animals` Bear 动画包第一帧制作，运行时输出为 540×420 RGBA；原始完整 64×33 帧按 10 倍最近邻放大。
- 来源页面：[OpenGameArt Animated Wild Animals](https://opengameart.org/content/animated-wild-animals)，记录授权为 CC0。
- 来源记录：[ArtSource/OpenGameArt/AnimatedWildAnimals/SOURCE.md](../ArtSource/OpenGameArt/AnimatedWildAnimals/SOURCE.md)。原始压缩包、原始动画条、裁剪帧和预览均保存在对应 `ArtSource/` 目录，不作为运行时路径。

### `Textures/Pixel/Heroes/`

- [hero_1_icon.png](Textures/Pixel/Heroes/hero_1_icon.png)：使用 Kenney Tiny Dungeon 的 `tile_0085` 制作沛总 UI 半身图标，运行时输出为 32×32 RGBA；原始 16×16 素材按 2 倍最近邻放大。
- 来源页面：[Kenney Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon)，记录授权为 CC0。
- 来源记录：[ArtSource/Kenney/TinyDungeon/SOURCE.md](../ArtSource/Kenney/TinyDungeon/SOURCE.md)。原始图块和授权文件保存在对应 `ArtSource/` 目录，不作为运行时路径。

### 其它当前无外部来源记录的目录

以下目录目前只有项目占位文件或没有已登记的第三方素材来源；后续加入外部资源时，应在对应目录小节下新增来源记录：

- `Config/`：项目运行时配置，来源和字段契约见本 README 的 Config 章节。
- `Textures/Pixel/Common/`
- `Textures/Pixel/Effects/`
- `Textures/Pixel/UI/`
- `Textures/Atlas/`
- `Animations/`
- `Audio/BGM/`
- `Audio/SFX/`
- `Fonts/`
- `Shaders/`

`ArtSource/` 已由根目录 `.gitignore` 忽略，但来源记录仍保留在本地，用于追溯授权和处理方式。原始素材、裁剪帧和预览属于来源记录，不应直接作为运行时路径或配置文件值。

## 资源验收清单

新增或替换资源时，至少检查：

1. 文件位于正确的 `Resources/` 分类目录，并没有把运行时文件放入 `ArtSource/` 或 `Classes/`。
2. 配置 JSON 可以解析，字段名称、版本、范围和资源 ID 符合对应契约。
3. PNG 尺寸、颜色模式、Alpha、像素密度和缩放采样方式符合本文件规范。
4. 透明边界没有黑边、白边或原素材背景色光晕。
5. 资源来源和授权记录已更新，运行时配置使用文件名/资源 ID 而不是编辑源路径。
6. 目标显示尺寸下的像素块观感与同一显示层的其它资源一致。
