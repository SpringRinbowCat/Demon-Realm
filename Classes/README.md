# Classes 模块

> 本文档中的路径均相对于 `Classes/` 目录；仓库根目录说明见 [../README.md](../README.md)，运行时资源说明见 [../Resources/README.md](../Resources/README.md)。

## 模块范围与当前状态

`Classes/` 是项目 C++ 代码根目录，负责游戏状态、业务规则、用例编排、Cocos 表现适配、存档和其它外部系统适配。

当前目录已经按目标架构建立，实现刚刚起步。已实现：

- `App/GameLauncher`：Cocos 应用启动入口与当前的组合根，设置渲染上下文、设计分辨率、帧率，加载运行时配置，装配战斗系统与首个场景，并持有业务对象的所有权。
- `Domain/Numeric/Decimal`：字符串定点数值类型，承载全部游戏数值。
- `Domain/Modifier/`：属性修正（buff）模型，`Modifier`、`ModifierAggregate` 与带缓存的 `ModifierCollection`。
- `Domain/State/`：`BossState`（血量与扣血）、`EconomyState`（金币与金币产出修正）、`HeroState`（基础属性、自身修正、自动攻击计时与派生属性缓存）。
- `Domain/Combat/CombatSystem`：自动攻击结算，按时间推进伤害与金币，并产出 `CombatTickReport`。
- `Application/BattleController`：战斗页面的用例入口，接收时间推进并产出展示快照。
- `Infrastructure/Config/ConfigService`：读取并校验 `Resources/Config/` 下的 Boss 与英雄配置，输出内存配置对象。
- `Presentation/MainSceneView`：单场景宿主的表现根节点，挂载当前页面、在页面之间切换、接收页面回传的请求，并把每帧时间推进转发给战斗用例。
- `Presentation/EnterGameView`：进入游戏页面，包含背景图和进入游戏按钮。
- `Presentation/BattleView`：战斗页面，包含顶部金币栏、上半区 Boss 区域、下半区可滑动英雄栏和底部栏入口，并按快照刷新金币与血量。
- `Presentation/Format/NumberFormatter`：数值展示格式化，千分位与秒数小数位裁剪。
- `Domain/Skill/SkillDefinition`：技能的领域定义，描述触发时机与效果参数。
- `Domain/Random/RandomSource` 与 `Infrastructure/Random/TimeSeededRandomSource`：概率判定用的随机源接口与按当前时间播种的实现。

尚未实现：`GameRoot`、升级、关卡推进、挂机离线收益、存档和除战斗页外的四个页面。下文中未标注为已实现的类名、数据流和目录职责属于目标设计，不代表现有代码已经完成。

项目已接入 cocos2d-x 4.0，macOS 上可以构建运行；平台入口在仓库根目录的 `proj.ios_mac/mac/main.cpp` 和 `proj.win32/main.cpp`，只负责创建 `GameLauncher` 并进入引擎主循环。构建步骤、架构限制和引擎补丁见 [../README.md](../README.md)。新增或移除源文件时必须同步更新根目录 `CMakeLists.txt` 的源文件列表。

代码约定：业务代码放在 `DemonRealm` 命名空间中，文件按 `.hpp` / `.cpp` 拆分，命名和边界规则见 `.kiro/skills/code-architecture-standards`。

Classes 不保存 PNG、JSON 或其它运行时资源。资源由 [Resources 模块](../Resources/README.md) 管理，Classes 只能通过配置、资源仓储或明确的 Infrastructure 接口访问它们。

## 游戏数值表示

所有游戏数值（攻击力、伤害、金币、血量、攻击间隔、buff 系数）统一使用 `Domain/Numeric/Decimal`：

- **存储形式**：十进制数字字符串，固定保留 4 位小数，位数只受内存限制。挂机数值会持续膨胀，`double` 会丢精度、`long long` 会溢出。
- **取整方向**：一律向下取整（截断），不做四舍五入。乘法先算出 8 位小数的中间结果，再截断回 4 位；除法按竖式长除法算到 4 位后截断，除数为 0 时返回失败而不是抛异常或给一个假结果。
- **非负约束**：只表示非负数，减法结果为负时钳制为 0。因此"减少类"效果（例如缩短攻击间隔）必须用小于 1 的乘法系数表达，不能用负的加法项。
- **配置来源**：`Resources/Config/` 里的数值字段写成字符串，解析阶段只校验格式，避免 JSON 数字被解析器先转成 `double`。详见 [Resources/README.md](../Resources/README.md)。
- **展示格式**：业务层只产出规范化字符串（例如 `"99999.0000"`），千分位、小数位裁剪等展示规则收口在 `Presentation/Format/NumberFormatter`。金币与血量按整数显示并加千分位；攻击间隔小数位最少 2 位、最多 4 位，去掉末尾多余的 0。

唯一的例外是**帧时间**：`HeroState` 的攻击计时用 `double` 秒数累加，只用于判断"是否到达攻击间隔"。帧时间来自引擎、量级很小且不参与任何产出计算；若把它也换成 4 位定点，每帧会固定丢掉一小截时间，累积成系统性的攻击变慢。

## 技能系统

技能由配置驱动，领域层只认识“触发时机 + 效果类型 + 效果参数”这三样东西，不为某个具体技能写专用分支：

- `SkillTrigger`：触发时机，当前只有 `TapAttack`（点击 Boss）。新增时机时在枚举里追加，并在战斗结算里补上对应入口。
- `SkillEffectType`：效果类型，当前有 `Damage`（造成攻击力 × 倍率的伤害）与 `PermanentAttackGrowth`（按概率永久提升攻击力）。
- 效果参数按类型取用 `SkillDefinition` 里对应的那一组，未知触发时机或效果类型在装配阶段就报错失败，不会加载出一个没有行为的技能。

技能定义不含展示文案。技能名等展示信息在 `Application/BattlePresentationData` 里，解锁判定在 `HeroState`，避免同一条规则两处各写一份。

### 点击攻击（已实现）

```text
BattleView 的 Boss 贴图命中区域收到触摸
    ↓ 回传点击事实
MainSceneView::_onBossTapped
    ↓
BattleController::onBossTapped
    ↓
CombatSystem::resolveTapAttack
    ├─ 遍历每个英雄的技能，跳过非点击类与未解锁的
    ├─ Damage：伤害 = 最终攻击力 × 倍率 → 走统一的 _applyDamage（扣血 + 按实际扣血量结算金币）
    └─ PermanentAttackGrowth：掷点命中后 HeroState::addPermanentAttackBonus
    ↓
CombatTickReport → RefreshRequest → 刷新金币、血量与英雄栏
```

规则要点：

- **点击本身不造成伤害**，伤害完全来自已解锁的点击类技能。没有解锁任何点击技能时点击不会有任何结果，这样"点击伤害"的数值口径始终由技能配置决定。
- **命中区域是 Boss 贴图自身的矩形**（当前 424×240，中心在设计坐标 y=700）。背景、血量文字和空白区域都不触发。触摸监听不吞掉事件，落在其他位置的触摸继续传给英雄栏和底部栏。
- 同一次点击会按配置顺序结算该英雄的全部点击技能，因此“先造成伤害、再判定成长”的顺序由配置里的技能顺序决定。
- Boss 血量归零后点击不再结算。

### 概率判定没有保底

`TimeSeededRandomSource` 每次取样都用当前的高精度时间重新播种，不复用长期存活的随机引擎：

- 每次点击都是独立事件，不累积“欠了多少次没触发”的状态，连续十次不触发与连续三次触发都是允许的结果。
- 复用一个长期引擎虽然分布更均匀，但那等价于一种隐式保底，与设计要求相反。
- 随机值精度是 4 位小数（一万个等概率取值），判定形式是 `随机值 < 概率`，所以配置 `0.2` 恰好对应 20%。

### 永久成长与 buff 的区别

成长类效果调用的是 `HeroState::addPermanentAttackBonus`，不是往修正集合里加 buff：

- 永久成长**不可移除**，会一直计入最终攻击力；buff 可以按来源 id 整批撤销。
- 计算顺序是 `最终攻击力 = (基础攻击力 + 永久成长 + 加法修正) × 乘法修正`，因此攻击力百分比 buff 对成长后的数值同样生效。
- 永久成长不经过修正集合，版本号不会变化，`HeroState` 因此额外维护一个脏标记；`resolveTapAttack` 在英雄属性变化后会立刻重算派生属性，避免界面显示上一次结算前的旧攻击力。

## 属性修正（buff）模型

伤害、金币产出和攻击间隔共用一套修正模型，新增 buff 不需要改战斗结算流程：

- `Modifier`：一条修正，包含来源 id、目标（`ModifierTarget`）、运算方式（加法或乘法）和数值。按来源 id 整批移除。
- `ModifierAggregate`：把任意多条修正压缩成"一个加法项 + 一个乘法项"，最终值固定按 `(基础值 + 加法项) × 乘法项` 结算，先加后乘是全局约定。
- `ModifierCollection`：修正容器，按目标缓存聚合结果，并维护版本号供外部做缓存失效判断。
- 作用范围：只影响单个英雄的 buff 写进该 `HeroState` 的修正集合；影响全体的写进 `CombatSystem::getGlobalModifiers()`；金币产出加成写进 `EconomyState` 或全局集合的 `GoldGain` 目标。

`HeroState` 的最终攻击力与最终攻击间隔是派生值，只在自身或全局修正版本变化后重算，平时每帧只做一次 `double` 比较，不触碰字符串大数运算。

## 架构原则

本模块遵循轻量分层、数据驱动和单一游戏状态源：

- Cocos Scene 只提供运行环境和根节点，不承载核心业务规则。
- `GameRoot` 是唯一组合根，负责创建对象、依赖注入、生命周期、存档和场景装配。
- 玩家、Boss、关卡、金币和升级状态由业务 Model 统一持有，View 不直接修改业务状态。
- 点击伤害和挂机伤害必须进入同一套 `CombatSystem` 规则，不能复制两套战斗逻辑。
- 只有具备独立职责、生命周期或存档边界的能力才拆成独立 System 或 Service。
- Boss、关卡、升级和奖励等平衡参数优先放在 [Resources/Config](../Resources/Config/) 中，不散落在 C++ 代码里。

## 分层职责与依赖方向

| 层 | 主要职责 | 关键约束 |
|---|---|---|
| `App` / `GameRoot` | 创建对象、依赖注入、生命周期和场景装配 | 作为唯一组合根，不承载具体玩法细节 |
| `Domain` / `Model` | 游戏状态和不依赖外部框架的纯 C++ 规则 | 不依赖 Cocos，不持有 `Node` 或 `Sprite` |
| `Application` / `Controller` | 将点击、购买、领取等输入编排为业务命令 | 不负责贴图、动画和具体 UI |
| `System` / `Service` | 战斗、经济、关卡、升级、挂机等可复用能力 | 单一职责，统一状态写入边界 |
| `Presentation` | Cocos 场景、输入、UI、动画和像素贴图表现 | 只展示状态、回传输入，不计算业务规则 |
| `Infrastructure` | 存档、配置、时间、平台、资源和音频适配 | 隔离 Cocos、文件系统和其它外部依赖 |
| `Events` | 跨模块的类型化事实通知 | 不用无约束字符串事件替代业务接口 |

依赖方向应保持单向：组合根装配各层，Application 调用 Domain/System，Presentation 通过 Controller 或查询接口进入业务，Infrastructure 实现外部适配。下层不得持有上层 View 或组合根对象，也不得通过全局查找绕过依赖注入。

## GameRoot 与核心系统

`GameRoot` 初期目标组合以下能力：

```text
GameRoot
├── CombatSystem       # 点击/挂机攻击、伤害和 Boss 战斗结果
├── EconomySystem      # 金币收入、消费和经济校验
├── StageSystem        # 关卡、Boss、过关和奖励配置
├── UpgradeSystem      # 数值升级、价格和升级效果
├── IdleSystem         # 在线挂机、离线时间和离线收益
├── SaveService        # 统一存档、版本迁移和脏数据刷盘
├── ConfigService      # Boss、关卡和升级配置读取与校验
├── GameViewAdapter    # Cocos 场景节点和业务 View 的装配
└── MainSceneView      # 单场景下的主界面和表现容器
```

这些能力不应被塞进一个万能 `GameManager` 或 `GameRoot`。`GameRoot` 只负责组合、所有权和生命周期，具体规则由对应的 Domain/System 承担。

## 核心数据流

### 点击攻击

```text
BossView / Cocos 输入
    ↓
CombatController
    ↓
CombatSystem
    ↓
BossState / EconomyState / StageState
    ↓
EventDamageApplied / EventGoldChanged / EventBossDefeated
    ↓
BossView / GoldView / StageView 刷新
```

### 自动攻击（已实现）

```text
MainSceneView::update（引擎帧时间）
    ↓
BattleController::advance
    ↓
CombatSystem::advance
    ├─ 按最新修正刷新英雄派生属性（版本未变则跳过）
    ├─ HeroState::advanceAutoAttack 累计计时并返回到期攻击次数
    ├─ 合并本帧攻击次数为一次伤害 → BossState::applyDamage（返回实际扣血量）
    └─ EconomyState::addGoldFromDamage（按实际扣血量与金币修正结算）
    ↓
CombatTickReport（是否有变化）
    ↓
BattleView::updateStatus 刷新金币与剩余血量
```

规则要点：

- 每个英雄每过一次攻击间隔造成一次等于最终攻击力的伤害，并获得等量金币（金币加成走 `GoldGain` 修正）。
- 金币按**实际扣血量**结算：Boss 只剩 1 点血时打出 5 点伤害，只获得 1 金币。
- Boss 血量归零后立即停止本次推进，`MainSceneView` 随后停止每帧推进，避免对着 0 血继续产出金币。关卡推进尚未实现。
- 单次推进的攻击次数有上限（`HeroState::kMaxAttacksPerAdvance`），防止进程被挂起很久后在一帧内结算过多攻击而卡住画面。
- 只有产生变化的那一帧才刷新界面，且视图会比对上一次的原始数值字符串，数值没变不触碰 `Label`。
- 进入游戏页停留期间不推进战斗，进入战斗页后才开始。

离线收益需要独立的时间源与批量补算（`GameClock` + `IdleSystem`），不能依赖上面的帧驱动追帧；`IdleSystem` 落地后只负责时间与批量结果，仍走同一套 `CombatSystem` 结算，不得复制另一套伤害或金币规则。

### 升级购买

```text
UpgradeView 点击
    ↓
UpgradeController
    ↓
UpgradeSystem 校验金币并提交升级
    ↓
EconomyState / UpgradeState 更新
    ↓
EventUpgradePurchased
    ↓
相关 View 刷新数值
```

## 命令、查询与事件

- 命令通过 Controller 或 System 的明确接口进入，例如 `CombatSystem::attack()`、`UpgradeSystem::purchase()`。
- 查询使用只读接口，View 不得穿透到任意底层对象修改状态。
- 通知使用类型化事件，例如 `EventDamageApplied`、`EventGoldChanged`、`EventBossDefeated` 和 `EventStageChanged`。
- EventBus 只承担“已经发生的事实”通知，不替代所有函数调用。
- 事件订阅必须绑定对象生命周期；模块退出或 View 卸载时取消订阅，避免悬空回调和重复处理。
- 共享状态必须有唯一写入边界，不能由多个系统无约束地同时修改同一字段。

## Cocos 与资源边界

- Cocos `Scene`、`Node`、`Sprite`、动画和触摸监听只属于 `Presentation` 层。
- `GameViewAdapter` 或 `GameViewLayout` 集中解析业务挂载点，避免业务代码散落 `getChildByName()` 和硬编码坐标。
- 像素贴图由 `PixelAssetRepository`、`PixelSpriteView` 或同职责资源模块统一管理。
- 贴图加载、像素过滤、整数倍缩放、Sprite Sheet、动画帧、缓存和释放属于 Presentation/Infrastructure 协作边界。
- 像素贴图只负责表现，不参与伤害、金币、升级和关卡规则计算。
- Domain/Model 不得包含 Cocos 头文件，也不得持有任何 Cocos 节点指针。

具体 PNG、JSON、像素密度和资源来源规则见 [Resources/README.md](../Resources/README.md)。

## 已实现页面与页面切换

当前有两个页面：进入游戏页面和战斗页面，都挂在同一个 `MainSceneView` 下，同一时间只挂载一个页面。

### 进入游戏页面

`Presentation/EnterGameView` 按三步交付：

1. **背景图**：加载页面专用背景 `Textures/Pixel/Backgrounds/背景_进入游戏界面.png`，在可见区域居中，按设计分辨率 1:1 呈现，不做运行时缩放，并对贴图设置最近邻采样。页面背景与战斗场景背景相互独立，不共用同一张贴图。
2. **进入游戏按钮**：用 `Textures/Pixel/UI/按钮_进入游戏_常态.png` 和 `按钮_进入游戏_按下.png` 组成 `MenuItemSprite`，挂在 `Menu` 上接收点击；按钮文字暂用系统字体绘制，接入像素字体后再替换。
3. **点击响应**：`_onEnterGameButtonClicked()` 当前只输出日志，并调用上层通过 `setOnEnterGameRequested()` 注入的回调；视图本身不决定跳转目标。

边界约束：

- 视图继承 `cocos2d::Node`，只做表现和输入回传，不持有业务状态，也不计算战斗、金币、升级或关卡规则。
- 按钮点击不直接切换场景；视图只回传“请求进入游戏”这一事实，真实跳转由上层实现。
- 视图内不启动线程、不持有异步句柄，只能在主线程创建、访问和销毁。
- 资源路径集中在实现文件的常量中，业务层不硬编码 Cocos 资源路径。
- 背景精灵和按钮菜单的生命周期由节点树持有，视图只保存非拥有引用。

### 战斗页面

`Presentation/BattleView` 按传入的 `BattleSnapshot` 铺设三块区域：

- **顶部金币栏**：金币图标加数量文字，数值来自经济状态，自动攻击产生收益时刷新。
- **Boss 区域**：只占屏幕上半区（设计坐标 y 480–960），包含战斗背景、Boss 待机贴图（中心 y=700）和剩余血量文字（y=520）。剩余血量随伤害结算刷新。
- **英雄栏**：占屏幕下半区（设计坐标 y 120–480），用 `cocos2d::ui::ScrollView` 垂直滚动，卡片按 `BattleSnapshot::heroes` 顺序从上往下排列。单卡 520×110，一屏显示三张，英雄变多时可上下滑动。
- **英雄卡片内文字**：共四行——第一行是名称与等级，第二行攻击力，第三行攻击间隔，第四行已解锁技能。名称与等级各自使用固定列坐标（卡片内 x=120 与 x=300），不靠空格拉开距离，因此不同名字长度的英雄之间名称对齐、等级也对齐。
- **底部栏**：五个入口按钮（战斗、英雄、商店、宝物、设置）。战斗为当前页，其余四个只回传点击事实，目标页面尚未实现。

`BattleSnapshot`（定义在 `Application/BattleSnapshot.hpp`）里全部是“已经算好的展示值”，数值是规范化定点小数字符串：视图不做等级成长、解锁判定或伤害计算，只负责格式化与呈现。等级到攻击力的分段成长属于 Domain 规则，尚未实现，因此初始等级展示的是配置里的基础攻击力。技能是否解锁由领域层的 `HeroState::isSkillUnlockedById` 判定，展示层只负责取出已解锁技能的名字。

快照分成两级，用来控制刷新开销：`BattleSnapshot` 含静态信息与英雄卡片，只在建立界面时产出一次；`BattleStatusSnapshot` 只含金币与剩余血量，仅在数值变化的那一帧产出。英雄属性变化（例如技能永久提升攻击力）时通过 `createHeroSnapshots` 单独刷新英雄栏。

刷新范围由 `BattleController::RefreshRequest` 描述：`status` 表示金币或血量变了，`heroes` 表示英雄属性变了。视图按范围只改对应的文字，英雄栏刷新不重建卡片节点，因此不会打断滚动位置。

### 页面装配与切换链路

```text
GameLauncher（渲染上下文、设计分辨率、帧率）
    ↓ ConfigService::load 读取 bosses.json / heroes.json
    ↓ 解析数值字符串，构造 CombatSystem（BossState / EconomyState / HeroState）
    ↓ 持有 BattleController（组合根拥有业务对象）
    ↓ runWithScene，向场景注入非拥有指针
MainSceneView（单场景宿主，持有 BattleController 的非拥有指针）
    ↓ addChild + setOnEnterGameRequested
EnterGameView（背景图 + 进入游戏按钮）
    ↓ 点击，回传“请求进入游戏”
MainSceneView 下一帧移除进入游戏页面、挂载 BattleView 并开始每帧推进
    ↓ setOnBottomBarItemSelected
BattleView（金币栏 + Boss + 英雄卡 + 底部栏）
    ↓ 底部栏点击，回传入口项
MainSceneView 决定页面切换（除战斗页外均未实现）
```

生命周期约束：

- 页面切换必须延迟到下一帧执行（`scheduleOnce`）。点击回调是在触摸分发过程中调用的，直接在回调里移除当前页面会销毁正在处理触摸的按钮节点。
- 业务对象由 `GameLauncher` 用 `std::unique_ptr` 持有，场景只拿非拥有指针。场景重建或页面切换不会丢失战斗进度，也不会造成业务对象被 Cocos 节点树间接释放。

待补齐项：`GameRoot` 尚未实现，目前由 `GameLauncher` 直接加载配置并装配业务对象；点击攻击、技能、升级、关卡推进、挂机离线收益和存档，以及底部栏其余四个页面都尚未实现。

## 存档与挂机时间

- 所有持久化统一经过 `SaveService`，业务模块通过明确的 Save Binding 注册自己的数据。
- 存档需要支持版本号、字段校验、旧版本迁移、写入失败处理和脏数据刷盘。
- 应用退后台、退出或关键奖励提交时，必须执行必要的立即刷盘。
- 挂机收益使用 `GameClock` 和 `lastActiveTimestamp` 计算，不依赖渲染帧率。
- 离线时间必须设置上限，并处理系统时间倒退、重复领取和数值溢出。
- 在线挂机可以使用固定逻辑 tick；UI 刷新频率不能决定业务结算频率。

## 通关重置与永久进度

目标规则由 Domain、Progression/Economy 和 Save 相关模块共同实现：

- 通关后，所有英雄等级重置为初始等级；英雄等级属于本轮成长数据。
- 通关后，清空常规货币 `currency` 的余额。
- `treasure` 掉落对应的宝物、解锁状态和永久加成在通关后保留。
- Boss 配置只描述掉落内容和概率；重置、保留和存档行为由业务与存档模块统一处理。

掉落字段和当前配置事实属于 Resources 文档，不在 Classes 中复制维护，详见 [Resources/README.md](../Resources/README.md)。

## 目标目录结构

以下目录相对于当前 `Classes/` 目录；目标类文件应按对应的 `.hpp` / `.cpp` 拆分。目前这些类尚未实现。

```text
./
├── App/
│   ├── GameRoot                 # 统一组装系统、依赖注入和生命周期
│   ├── GameLauncher             # Cocos 启动入口（已实现）
│   └── GameLifecycle            # 前后台、暂停、恢复和退出处理
├── Domain/
│   ├── Numeric/                 # 定点数值类型 Decimal（已实现）
│   ├── Modifier/                # 属性修正模型与聚合缓存（已实现）
│   ├── Skill/                   # 技能定义：触发时机与效果参数（已实现）
│   ├── Random/                  # 概率判定用的随机源接口（已实现）
│   ├── State/                   # 玩家、Boss、关卡、金币和升级状态（已实现 Boss/Economy/Hero）
│   ├── Combat/                  # 点击、挂机、离线攻击和伤害结算（已实现自动攻击）
│   ├── Economy/                 # 金币收入、消费和经济规则
│   ├── Progression/             # 关卡推进、解锁和通关奖励
│   └── Idle/                    # 在线挂机、离线时间和离线收益
├── Application/
│   ├── BattleController          # 战斗页面用例入口与展示快照（已实现）
│   ├── BattleSnapshot            # 战斗页面展示快照结构（已实现）
│   ├── BattlePresentationData    # 与战斗推进无关的展示信息（已实现）
│   ├── UpgradeController         # 处理升级购买请求
│   ├── StageController           # 处理关卡进入、切换和重试
│   └── IdleController            # 处理自动战斗和离线收益领取
├── Presentation/                 # Cocos 场景、UI、输入和动画表现
│   ├── EnterGameView             # 进入游戏页面背景与进入游戏按钮（已实现）
│   ├── BattleView                # 战斗页面金币栏、Boss、英雄卡与底部栏（已实现）
│   ├── Format/                   # 数值展示格式化（已实现 NumberFormatter）
│   ├── MainSceneView             # 单场景表现根节点（已实现）
│   ├── BossView                  # Boss 贴图、血条和受击表现
│   ├── GoldView                  # 金币数量和奖励表现
│   ├── UpgradeView               # 升级项目、等级和价格界面
│   ├── StageView                 # 关卡和通关状态界面
│   ├── PixelSpriteView           # 像素 Sprite 和帧动画表现
│   └── GameViewAdapter           # 场景节点与各 View 的装配
├── Infrastructure/
│   ├── Save/                     # 存档、加载和版本迁移
│   ├── Config/                   # Boss、关卡和升级配置读取（已实现 ConfigService）
│   ├── Time/                     # 游戏时间和挂机时间来源
│   ├── PixelAssets/              # 像素贴图加载、缓存和释放
│   └── Platform/                 # Windows/macOS 平台差异封装
└── Events/
    ├── CombatEvents              # 攻击、伤害和 Boss 击杀事件
    ├── EconomyEvents             # 金币和奖励变化事件
    └── ProgressionEvents         # 关卡推进和升级事件
```

## 架构红线

1. 不把 Cocos `Node` 当作游戏数据模型。
2. 不把所有逻辑塞进一个 `GameManager` 或 `GameRoot`。
3. 不为点击和挂机分别实现两套战斗规则。
4. 不让 View 直接修改金币、Boss 血量、升级和关卡状态。
5. 不让每个模块自行读写存档。
6. 不让像素贴图资源依赖 Domain 规则。
7. 不为了模仿大型项目而过早拆分过多系统。
8. 不用 `float`、`double` 或整数类型保存游戏数值，一律使用 `Decimal`；帧时间是唯一例外。
9. 不绕过 `CombatSystem` 直接改血量或金币，也不在结算之外自行计算 buff 加成。

新增代码时，先确认职责归属、依赖方向、所有权、生命周期、线程边界和错误处理，再扩展对应目录；不要用临时全局状态、跨层指针或 View 特判绕过架构边界。
