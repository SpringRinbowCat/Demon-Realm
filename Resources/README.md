# Resources 模块

> 本文档中的路径均相对于 `Resources/` 目录；仓库根目录说明见 [../README.md](../README.md)，代码架构说明见 [../Classes/README.md](../Classes/README.md)。

## 模块范围与当前状态

`Resources/` 保存游戏运行时读取的配置、图片、动画、音频、字体和 Shader 资源。它不保存 C++ 代码，也不承载伤害、金币、升级、关卡或存档规则的实现。

当前资源目录已经建立基础分类，但仍处于第一版状态：实际可用配置主要是 `Config/bosses.json` 和 `Config/heroes.json`，运行时 PNG 包括火焰背景、阿熊待机图、沛总 UI 图标和进入游戏按钮的两张状态贴图，其余目录多数只有 `.gitkeep`。

资源加载、解析、缓存和释放由 [Classes/Infrastructure](../Classes/Infrastructure/) 中目标的 Infrastructure 模块负责；当前这些模块尚未实现，文档中的服务名称属于目标职责，进入游戏页面暂时直接用引擎接口按路径加载贴图。

打包方式：构建时 `Resources/` 的内容会整体复制进应用包（macOS 为 `DemonRealm.app/Contents/Resources/`），因此代码和配置中的资源路径都以 `Resources/` 为根，例如 `Textures/Pixel/UI/按钮_进入游戏_常态.png`。

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
│   │   │   └── Boss_1.png
│   │   ├── Heroes/
│   │   │   ├── hero_1.png          # UI 图标，2 倍密度
│   │   │   └── hero_1_卡片.png     # 战斗页卡片立绘，10 倍密度
│   │   ├── UI/                     # 按钮、图标和面板贴图
│   │   │   ├── 按钮_进入游戏_常态.png
│   │   │   ├── 按钮_进入游戏_按下.png
│   │   │   ├── 按钮_底部栏_常态.png
│   │   │   ├── 按钮_底部栏_按下.png
│   │   │   ├── 图标_金币.png
│   │   │   └── 面板_英雄卡片.png
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

## 资源命名规范

运行时图片使用「类型_用途」的下划线命名，便于按名称直接定位和替换素材：

- 背景：`背景_<界面名>.png`，例如 `背景_进入游戏界面.png`、`背景_战斗界面.png`。
- Boss：`Boss_<编号>.png`，例如 `Boss_1.png`。
- 英雄：`hero_<编号>.png`，例如 `hero_1.png`。
- 界面按钮：`按钮_<用途>_<状态>.png`，例如 `按钮_进入游戏_常态.png`、`按钮_底部栏_按下.png`。
- 界面图标：`图标_<用途>.png`，例如 `图标_金币.png`。
- 界面面板：`面板_<用途>.png`，例如 `面板_英雄卡片.png`。

约定：

- 分隔符统一使用下划线，不使用空格或连字符。
- 中文文件名保持 UTF-8 NFC 形式；引用这些文件的 C++ 源文件必须以 UTF-8 保存，Windows 构建已在 `CMakeLists.txt` 中通过 `/utf-8` 指定窄字符串字面量编码。
- 配置文件的图片字段只写文件名，不写目录；目录由资源分类决定。
- 同一资源出现多个状态或帧时，在用途后追加状态段，例如 `_常态`、`_按下`。
- 同一角色在不同显示层需要不同像素密度时，用途段区分用法，例如 UI 图标 `hero_1.png` 与战斗页卡片立绘 `hero_1_卡片.png`。

## 运行时资源与 ArtSource 边界

- `Resources/` 只保存运行时需要读取或打包的资源。
- `ArtSource/` 只保存 PSD、Aseprite、原始压缩包、来源记录和其它美术编辑源文件，不参与运行时打包。
- `ArtSource/` 已由根目录 `.gitignore` 忽略；来源记录仍保留在本地，用于追溯授权和处理方式。
- `Classes/Infrastructure/PixelAssets/` 只保存资源访问、缓存和释放代码，不保存 PNG。
- 配置中的图片值是资源 ID 或文件名，不是 Markdown 链接，也不能因为 README 的位置变化而机械添加 `../`。实际路径由配置/资源适配层解析。

当前资源入口：

- [战斗火焰背景](Textures/Pixel/Backgrounds/背景_战斗界面.png)：540×960，无 Alpha；用于 Boss 战斗场景。
- [进入游戏页背景](Textures/Pixel/Backgrounds/背景_进入游戏界面.png)：540×960，无 Alpha；魔域门扉构图，仅用于进入游戏页面。
- [阿熊待机图](Textures/Pixel/Bosses/Boss_1.png)：424×240，RGBA；Bear 64×33 帧按不透明区域裁切为 53×30 后按 8 倍最近邻放大。
- [沛总 UI 图标](Textures/Pixel/Heroes/hero_1.png)：32×32，RGBA；使用 16×16 源图按 2 倍最近邻放大。
- [进入游戏按钮常态](Textures/Pixel/UI/按钮_进入游戏_常态.png)：300×90，RGBA；源图 30×9 像素格按 10 倍最近邻放大。
- [进入游戏按钮按下态](Textures/Pixel/UI/按钮_进入游戏_按下.png)：300×90，RGBA；与常态同规格，明暗关系相反。
- [底部栏按钮常态](Textures/Pixel/UI/按钮_底部栏_常态.png) 与 [按下态](Textures/Pixel/UI/按钮_底部栏_按下.png)：100×100，RGBA；源图 10×10 像素格按 10 倍放大。五个按钮加四个 10px 间隔正好铺满 540 宽。
- [金币图标](Textures/Pixel/UI/图标_金币.png)：60×60，RGBA；源图 6×6 像素格按 10 倍放大。
- [英雄卡片面板](Textures/Pixel/UI/面板_英雄卡片.png)：520×110，RGBA；源图 52×11 像素格按 10 倍放大。卡片高度按英雄栏一屏显示三张设计。
- [沛总战斗卡片立绘](Textures/Pixel/Heroes/hero_1_卡片.png)：80×80，RGBA；16×16 源图按 5 倍最近邻放大，适配压缩后的卡片高度。

进入游戏页面和战斗场景使用各自独立的背景资源，不共用同一张图：页面背景为 `背景_进入游戏界面.png`，战斗背景为 `背景_战斗界面.png`。进入游戏按钮落在页面背景下半的低干扰地面区。

## Config 配置契约

目标加载边界是 `Classes/Infrastructure/Config/ConfigService`：负责加载、解析、版本校验、字段范围校验和错误处理；配置文件只描述数据，不实现战斗、升级或存档流程。

### 数值字段一律写成字符串

游戏数值（血量、攻击力、攻击间隔、掉落数量、概率、升级费用与倍率）在 JSON 里必须写成字符串，例如 `"maxHp": "100000"`、`"baseAttackIntervalSeconds": "4.0"`。

原因：挂机游戏的数值会膨胀到 double 无法精确表示的量级，而 JSON 数字会被解析器先转成 double，精度在读配置这一步就已经丢了。写成字符串后，解析阶段只校验格式，真正的计算交给 `Domain/Numeric/Decimal`（定点小数，固定 4 位小数、一律向下取整）。

例外：等级、区间边界、`schemaVersion` 这类不会膨胀的计数仍写成 JSON 整数（`minLevel`、`maxLevel`、`unlockLevel`）。

小数位超过 4 位的部分会在解析时被向下取整截断，例如 `"2.43281"` 实际按 `2.4328` 生效。

### Boss 配置

当前 [Config/bosses.json](Config/bosses.json) 为 `schemaVersion` 1，包含一个 id 为 `1`、名称为“阿熊”的 Boss，初始 `maxHp` 为 `"100000"`。

每个 `drops` 条目包含：

- `type`：只能是 `currency` 或 `treasure`。
- `itemId`：掉落物或宝物 ID。
- `quantity`：掉落数量，十进制字符串。
- `probability`：0 到 1 之间的掉落概率，十进制字符串。

`currency` 是本轮使用的常规货币，通关结算后清空；`treasure` 对应通关后保留的宝物、解锁状态或永久加成。Boss 的 `images` 当前包含 `background` 和 `idle` 两个文件名字段。

### 英雄配置

当前 [Config/heroes.json](Config/heroes.json) 为 `schemaVersion` 1，包含 id 为 `1` 的“沛总”。英雄对象中的 `attackLevelMultiplierRanges` 用于配置攻击力等级的分段成长倍率：

- `minLevel` 和 `maxLevel` 都包含边界。
- `multiplier` 作用于该区间内攻击力等级的成长。
- 区间不能重叠，且 `minLevel` 不得大于 `maxLevel`。
- 英雄等级和攻击力等级是两个独立概念。

当前配置为攻击力等级 1–30 倍率 `"2.0"`、31–80 倍率 `"1.5"`。`upgradeCostMultiplier` 只表示升级费用增长，不表示攻击力成长倍率。

`baseAttack` 与 `baseAttackIntervalSeconds` 是战斗系统实际读取的字段：前者是单次攻击造成的伤害，后者是自动攻击的间隔秒数，两者都必须大于 0。当前配置为 `"1"` 与 `"4.0"`，即每 4 秒造成 1 点伤害并获得 1 金币。

英雄的 `images` 包含两个字段：`icon` 是 UI 图标（2 倍密度），`card` 是战斗页卡片立绘（10 倍密度）。

### 技能配置

`skills` 每项包含 `id`、`unlockLevel`、`displayName`、`description`、`trigger` 和 `effect`。战斗页只展示 `unlockLevel` 不大于当前英雄等级的技能名称，也只有已解锁的技能会真的生效。

`trigger` 是触发时机，当前只支持：

- `tapAttack`：玩家点击 Boss 时触发。命中区域是 Boss 贴图自身的矩形范围。

`effect` 是效果描述，`type` 决定需要哪些参数，参数一律写成字符串：

- `damage`：造成伤害。需要 `attackMultiplier`（伤害倍率，作用在英雄最终攻击力上；`"1.0"` 等同于一次普通攻击）。
- `permanentAttackGrowth`：按概率永久提升攻击力。需要 `chance`（触发概率，取值 0 到 1）和 `levelProductDivisor`（除数，必须大于 0）。提升量固定按 `攻击力等级 × 英雄等级 ÷ levelProductDivisor` 计算。

未知的 `trigger` 或 `effect.type` 会让配置加载失败，避免上线一个不产生任何效果的技能。`chance` 超过 1 也会被拒绝。

概率没有保底：每次点击都按当前时间独立取随机数，不记录“连续多少次没触发”。所以 `"0.2"` 就是每次点击各自 20% 的概率，不保证若干次内必定触发。

当前沛总的两个技能：`keyboard_face_roll` 解锁等级 1，倍率 `"1.0"`；`growth` 解锁等级 20，概率 `"0.2"`、除数 `"50"`。攻击力等级由升级系统推进，升级尚未实现，目前恒为 1。

`Classes/Infrastructure/Config/ConfigService` 会校验 `schemaVersion`、必填字符串、数值字符串的格式与正负、`images` 子字段和 `skills` 数组；任一项不合法都会记录日志并整体加载失败，不会保留半份配置。数值字段的原始字符串会原样传给业务层，解析服务自身不做单位换算或数值转换。

## 像素图片资源规范

为避免 Boss、人物和背景的像素密度混用，运行时像素资源按显示层分档：

像素密度按显示层分档，同一层内必须一致：

- **背景层（10 倍）**：540×960 背景对应 54×96 个逻辑格。
- **场景层（8 倍）**：Boss 和场景内角色使用 8 倍最近邻，并按不透明区域紧裁切，不保留大片透明画布。Boss 必须能完整落在屏幕上半区，例如 Bear 的 64×33 原始帧裁切为 53×30 后按 8 倍放大到 424×240。
- **UI 面板层（10 倍）**：按钮、图标和面板等界面框体使用 10 倍，与背景保持同一格宽。
- **面板内头像（5 倍）**：英雄栏卡片里的立绘使用 5 倍，例如 16×16 源图放大到 80×80，让卡片能压缩到列表可容纳的高度。
- **非战斗界面小图标（2 倍）**：`hero_1.png` 这类 16×16 源图按 2 倍放大到 32×32，只用于战斗页以外的界面。

背景 10 倍与场景 8 倍是有意的分档：Boss 若按 10 倍会超出上半区，而运行时非整数缩放会破坏像素边缘，所以改为在导出阶段降一档密度。
- **全屏页面按钮**：进入游戏等按钮与全屏背景同屏显示，必须使用 10×10 输出像素格，例如 30×9 像素格源图按 10 倍最近邻放大到 300×90，不与 2 倍的卡片图标混用。按钮文字在接入像素字体前可用系统字体绘制，但贴图本身不得做非整数缩放。
- **缩放与导出**：只能使用整数倍缩放和最近邻采样；禁止双线性、双三次、抗锯齿和运行时非整数缩放。源图尺寸、裁剪边界和场景摆放位置应对齐对应像素格。
- **透明与边缘**：Boss、人物和特效使用 RGBA；轮廓外保持透明，不留黑边、白边或原素材背景色的 matte 光晕。背景使用完整画布，不把人物背景色烘焙进背景图。
- **色阶与邻接像素**：同一背景区域使用有限调色板；相邻像素格只有在火焰轮廓、角色轮廓等有意边界处才允许出现明显色差，不用随机渐变制造细碎跳色。火焰带内部的近黑填充统一使用与上方暗红衔接的 RGB(70,6,12)，下半遮挡区 y≥400 固定为 RGB(145,14,16)。
- **背景构图**：以下坐标自顶部计算。顶部 0–112px 为金币栏等系统 UI 的暗色安全区；主要火光和 Boss 对比区域位于上半屏；y≥400 的下半区为单一遮挡色，不放置必须被看清的火光、角色轮廓或关键特效。战斗页的英雄栏占据自顶 480–840px、底部栏占据 840–960px，这两段一定会被 UI 覆盖。
- **验收方式**：必须在目标显示尺寸下检查背景、Boss 和人物是否拥有相同的像素块观感；同时检查 PNG 尺寸、Alpha、缩放方式和来源授权记录。

## 资源来源与授权

来源记录按照运行时资源目录分层维护。每个运行时目录只记录该目录中资源的来源、授权和对应的 `ArtSource/` 记录，避免不同资源类型混在一起。

### `Textures/Pixel/Backgrounds/`

- [背景_战斗界面.png](Textures/Pixel/Backgrounds/背景_战斗界面.png)：项目内原创/处理资源，当前没有第三方来源记录。
- [背景_进入游戏界面.png](Textures/Pixel/Backgrounds/背景_进入游戏界面.png)：项目内原创资源，没有第三方来源。由 54×96 像素格程序化生成并按 10 倍最近邻放大，构图为居中的魔域门扉与门内火光；调色板取自项目既有红/琥珀色系（天空 RGB(14,2,5)、RGB(28,4,8)、RGB(45,6,10)，门框 RGB(24,3,8) 与 RGB(52,7,12)，火光 RGB(145,14,16)、RGB(238,56,9)、RGB(255,173,28)、RGB(255,220,61)，地面 RGB(35,5,9)），共 12 色。

### `Textures/Pixel/Bosses/`

- [Boss_1.png](Textures/Pixel/Bosses/Boss_1.png)：使用 OpenGameArt.org 的 `Animated Wild Animals` Bear 动画包第一帧制作，运行时输出为 424×240 RGBA；原始 64×33 帧按不透明区域裁切为 53×30 后按 8 倍最近邻放大。
- 来源页面：[OpenGameArt Animated Wild Animals](https://opengameart.org/content/animated-wild-animals)，记录授权为 CC0。
- 来源记录：[ArtSource/OpenGameArt/AnimatedWildAnimals/SOURCE.md](../ArtSource/OpenGameArt/AnimatedWildAnimals/SOURCE.md)。原始压缩包、原始动画条、裁剪帧和预览均保存在对应 `ArtSource/` 目录，不作为运行时路径。

### `Textures/Pixel/Heroes/`

- [hero_1.png](Textures/Pixel/Heroes/hero_1.png)：使用 Kenney Tiny Dungeon 的 `tile_0085` 制作沛总 UI 半身图标，运行时输出为 32×32 RGBA；原始 16×16 素材按 2 倍最近邻放大。
- [hero_1_卡片.png](Textures/Pixel/Heroes/hero_1_卡片.png)：同一 `tile_0085` 素材，按 5 倍最近邻放大为 80×80 RGBA，用于战斗页英雄栏卡片内的立绘。
- 来源页面：[Kenney Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon)，记录授权为 CC0。
- 来源记录：[ArtSource/Kenney/TinyDungeon/SOURCE.md](../ArtSource/Kenney/TinyDungeon/SOURCE.md)。原始图块和授权文件保存在对应 `ArtSource/` 目录，不作为运行时路径。

### `Textures/Pixel/UI/`

- [按钮_进入游戏_常态.png](Textures/Pixel/UI/按钮_进入游戏_常态.png) 与 [按钮_进入游戏_按下.png](Textures/Pixel/UI/按钮_进入游戏_按下.png)：项目内原创资源，没有第三方来源。由 30×9 像素格程序化生成并按 10 倍最近邻放大，颜色取自火焰背景既有调色板：轮廓 RGB(24,3,8)、常态填充 RGB(238,56,9)、高光 RGB(255,173,28)、暗部 RGB(145,17,8)。
- [按钮_底部栏_常态.png](Textures/Pixel/UI/按钮_底部栏_常态.png)、[按钮_底部栏_按下.png](Textures/Pixel/UI/按钮_底部栏_按下.png)、[图标_金币.png](Textures/Pixel/UI/图标_金币.png) 与 [面板_英雄卡片.png](Textures/Pixel/UI/面板_英雄卡片.png)：项目内原创资源，没有第三方来源。均由像素格程序化生成并按 10 倍最近邻放大，取自同一调色板：轮廓 RGB(24,3,8)、面板填充 RGB(45,5,9)、面板高光 RGB(70,6,12)、金币 RGB(255,173,28) 与 RGB(255,220,61)。

### 其它当前无外部来源记录的目录

以下目录目前只有项目占位文件或没有已登记的第三方素材来源；后续加入外部资源时，应在对应目录小节下新增来源记录：

- `Config/`：项目运行时配置，来源和字段契约见本 README 的 Config 章节。
- `Textures/Pixel/Common/`
- `Textures/Pixel/Effects/`
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
