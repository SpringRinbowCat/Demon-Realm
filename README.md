# 魔域（Demon-Realm）

## 基本项目约定

### 1. 沟通称呼

与用户沟通时，统一称呼用户为“焜爷”。

### 2. 项目背景与初步玩法

- 当前项目是《魔域》（Demon-Realm），是一款兼容 Windows/macOS 的可安装点击/挂机游戏。
- 当前初步玩法是：玩家通过点击或挂机对关卡敌人（Boss）造成伤害并获得金币。
- 玩家使用金币提升数值，并不断推进关卡。
- 以上内容是当前项目背景和玩法基线；未明确的设计细节，以用户后续确认、项目文档和现有代码为准，不得擅自推断为确定规则。

### 3. 技术架构

- 运行框架：Cocos。
- 核心逻辑：使用 C++ 实现。
- 画面表现：采用像素格贴图方式实现，以像素网格作为贴图的基本构成单位。
- 当前项目技术组合为：Cocos + C++ + 像素格贴图。

## 目标架构

本项目采用**轻量分层架构 + 数据驱动 + 单一游戏状态源**。以下是当前确认的目标结构，后续代码实现应逐步遵守；未实现的部分不视为已经存在的代码事实。

### 总体原则

- **单场景宿主**：Cocos Scene 只负责提供运行环境和根节点，不承载核心业务规则。
- **单一组合根**：由 `GameRoot` 统一创建、持有和销毁核心系统，负责依赖注入、生命周期、存档和场景挂载。
- **单一状态源**：玩家、Boss、关卡、金币和升级状态由业务 Model 统一持有，View 不直接修改业务状态。
- **统一战斗入口**：点击伤害和挂机伤害必须进入同一套 `CombatSystem` 规则，避免两套数值逻辑不一致。
- **轻量分层**：不直接复制大型项目的复杂度；只有具备独立职责、生命周期或存档边界的功能才拆成独立 System。
- **数据驱动**：Boss、关卡、升级、金币奖励等平衡参数优先放在配置数据中，避免散落在 C++ 业务代码中。

### 分层职责

| 层 | 主要职责 | 关键约束 |
|---|---|---|
| `GameRoot` / App | 创建对象、依赖注入、生命周期、场景装配 | 作为唯一组合根，不承载具体玩法细节 |
| Domain / Model | 游戏状态和纯 C++ 规则 | 不依赖 Cocos，不持有 `Node`/`Sprite` |
| Application / Controller | 把点击、购买、领取等输入编排为业务命令 | 不负责具体贴图和动画 |
| System / Service | 战斗、经济、关卡、升级、挂机等可复用业务能力 | 单一职责，统一状态写入边界 |
| Presentation / Cocos | 场景、输入、UI、动画和像素格贴图表现 | 只展示状态、回传输入，不计算业务规则 |
| Infrastructure | 存档、配置、时间、平台、资源和音频适配 | 通过接口供业务层使用，隔离外部依赖 |

### 核心系统

`GameRoot` 初期建议组合以下模块：

```text
GameRoot
├── CombatSystem       # 点击/挂机攻击、伤害和 Boss 战斗结果
├── EconomySystem      # 金币收入、消费和经济校验
├── StageSystem        # 关卡、Boss、过关和奖励配置
├── UpgradeSystem      # 数值升级、价格和升级效果
├── IdleSystem         # 在线挂机、离线时间和离线收益
├── SaveService        # 统一存档、版本迁移和脏数据刷盘
├── ConfigService      # Boss、关卡和升级配置
├── GameViewAdapter    # Cocos 场景节点和业务 View 的装配
└── MainSceneView      # 单场景下的主界面和表现容器
```

### 核心数据流

点击攻击：

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

挂机攻击：

```text
GameClock
    ↓
IdleSystem 计算在线或离线时间结果
    ↓
批量生成统一的攻击/伤害结算
    ↓
CombatSystem
    ↓
统一处理 Boss、金币和关卡推进
```

升级购买：

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

### 事件、命令与查询

- **命令**通过 Controller 或 System 的明确接口进入，例如 `CombatSystem::attack()`、`UpgradeSystem::purchase()`。
- **查询**使用只读接口，禁止 View 穿透到任意底层对象修改状态。
- **通知**使用类型化事件，例如 `EventDamageApplied`、`EventGoldChanged`、`EventBossDefeated` 和 `EventStageChanged`。
- EventBus 只承担“已经发生的事实”通知，不替代所有函数调用，也不使用无约束的字符串事件作为业务总线。
- 事件订阅必须绑定对象生命周期；模块退出或 View 卸载时取消订阅。

### Cocos 与像素格贴图边界

- Cocos `Scene`、`Node`、`Sprite`、动画和触摸监听只属于 Presentation 层。
- `GameViewAdapter` / `GameViewLayout` 集中解析业务挂载点，避免业务代码散落 `getChildByName()` 和硬编码坐标。
- 像素格贴图由 `PixelAssetRepository`、`PixelSpriteView` 或同职责资源模块统一管理。
- 像素资源需要统一处理像素格尺寸、最近邻过滤、整数倍缩放、Sprite Sheet、动画帧、缓存和释放。
- 像素贴图只负责表现，不参与伤害、金币、升级和关卡规则计算。
- Domain/Model 不得包含 Cocos 头文件，也不得持有任何 Cocos 节点指针。

### 存档和挂机时间

- 所有持久化统一经过 `SaveService`，业务模块通过明确的 Save Binding 注册自己的数据。
- 存档需要支持版本号、字段校验、旧版本迁移、写入失败处理和脏数据刷盘。
- 应用退后台、退出或关键奖励提交时，必须执行必要的立即刷盘。
- 挂机收益使用 `GameClock` 和 `lastActiveTimestamp` 计算，不依赖渲染帧率。
- 离线时间必须设置上限，并处理系统时间倒退、重复领取和数值溢出。
- 在线挂机可以使用固定逻辑 tick；UI 刷新不应决定业务结算频率。

### 推荐目录结构

以下目录和文件为目标结构，实际 C++ 文件按对应的 `.hpp` / `.cpp` 拆分。

```text
Classes/                                      # C++ 代码根目录
├── App/                                      # 应用入口、组合根和生命周期
│   ├── GameRoot                              # 统一组装系统、依赖注入和生命周期
│   ├── GameLauncher                          # Cocos 启动入口
│   └── GameLifecycle                         # 前后台、暂停、恢复和退出处理
├── Domain/                                   # 游戏状态和纯 C++ 业务规则
│   ├── State/                                # 玩家、Boss、关卡、金币和升级状态
│   ├── Combat/                               # 点击、挂机、离线攻击和伤害结算
│   ├── Economy/                              # 金币收入、消费和经济规则
│   ├── Progression/                          # 关卡推进、解锁和通关奖励
│   └── Idle/                                 # 在线挂机、离线时间和离线收益
├── Application/                              # 将用户输入编排为业务命令
│   ├── CombatController                      # 接收攻击输入并调用 CombatSystem
│   ├── UpgradeController                     # 处理升级购买请求
│   ├── StageController                       # 处理关卡进入、切换和重试
│   └── IdleController                        # 处理自动战斗和离线收益领取
├── Presentation/                             # 画面表现总目录
│   └── Cocos/                                # Cocos 场景、UI、输入和动画
│       ├── MainSceneView                     # 单场景表现根节点
│       ├── BossView                          # Boss 贴图、血条和受击表现
│       ├── GoldView                          # 金币数量和奖励表现
│       ├── UpgradeView                       # 升级项目、等级和价格界面
│       ├── StageView                         # 关卡和通关状态界面
│       ├── PixelSpriteView                   # 像素 Sprite 和帧动画表现
│       └── GameViewAdapter                   # 场景节点与各 View 的装配
├── Infrastructure/                           # 外部系统和平台适配
│   ├── Save/                                 # 存档、加载和版本迁移
│   ├── Config/                               # Boss、关卡和升级配置读取
│   ├── Time/                                 # 游戏时间和挂机时间来源
│   ├── PixelAssets/                          # 像素贴图加载、缓存和释放
│   └── Platform/                             # Windows/macOS 平台差异封装
└── Events/                                   # 跨模块类型化事件
    ├── CombatEvents                          # 攻击、伤害和 Boss 击杀事件
    ├── EconomyEvents                         # 金币和奖励变化事件
    └── ProgressionEvents                     # 关卡推进和升级事件
```

### 运行时资源目录结构

运行时资源放在项目根目录的 `Resources/` 下，与 `Classes/` 同级，不放入 C++ 代码目录。美术编辑源文件可以放在可选的 `ArtSource/` 目录中，不参与运行时打包。

```text
Demon-Realm/
├── Classes/                  # C++ 代码
├── Resources/                # 游戏运行时资源
│   ├── Config/               # Boss、关卡、升级和奖励等配置
│   ├── Textures/
│   │   ├── Pixel/            # 像素格 PNG 贴图
│   │   │   ├── Bosses/
│   │   │   ├── Player/
│   │   │   ├── UI/
│   │   │   ├── Effects/
│   │   │   ├── Backgrounds/
│   │   │   └── Common/
│   │   └── Atlas/            # Sprite Sheet、图集及其描述文件
│   ├── Animations/           # 帧动画和动画配置
│   ├── Audio/                # 背景音乐和音效
│   ├── Fonts/                # 像素字体和 UI 字体
│   └── Shaders/              # 受击、描边、发光等 Shader
├── ArtSource/                # 可选：PSD、Aseprite 等美术源文件
├── proj.win32/               # Windows 工程目录
├── proj.ios_mac/             # macOS 工程目录
└── README.md
```

资源与代码的边界如下：

- `Resources/Config/` 保存运行时读取的 Boss、关卡、升级和奖励配置，由 `Infrastructure/Config/ConfigService` 负责加载、解析和校验。
- `Resources/Config/bosses.json` 当前只配置 id 为 `1`、名称为 `阿熊` 的 Boss，包含初始 `maxHp`、掉落列表和图片资源字段；当前数值属于第一版配置，可继续调整。
- `bosses.json` 的 `drops` 每项包含 `type`、`itemId`、`quantity` 和 `probability`，其中 `probability` 使用 `0` 到 `1` 表示掉落概率。
- `drops.type` 只允许两种类型：`currency` 表示本轮使用的常规货币，`treasure` 表示可形成永久加成的宝物。
- `currency` 掉落在通关结算后清空；`treasure` 掉落对应的宝物和永久加成在通关后保留。
- `bosses.json` 的 `images` 当前只包含 `background` 和 `idle`；火焰背景使用 `demon_realm_fire_background.png`，阿熊待机图使用 `boss_1_idle.png`。
- `demon_realm_fire_background.png` 为 540x960 的 9:16 竖屏背景，顶部约 112px 保持暗色安全区，用于放置金币栏；火焰主要位于下半区域。
- `boss_1_idle.png` 为 360x420 的透明像素图，参考真实棕熊的低头、小圆耳、厚肩峰、桶状胸腹、沉重前肢和短粗后腿来塑造 Boss 体量；通过抬掌、扑桌和散落麻将牌表现活跃撒野，不使用玩偶式人形四肢、鞋子式脚掌，也不要求露齿或显示尖爪，设计用于金币栏下方的背景上半区域。
- `Resources/Textures/Pixel/` 保存像素格贴图；`Resources/Textures/Atlas/` 保存 Sprite Sheet、图集及相关描述文件。
- `Resources/Animations/` 保存动画帧定义和动画配置，不承载战斗或升级规则。
- `Resources/Audio/`、`Resources/Fonts/` 和 `Resources/Shaders/` 分别保存音频、字体和运行时 Shader 资源。
- `ArtSource/` 只保存美术编辑源文件，不作为游戏运行时资源打包。
- `Classes/Infrastructure/PixelAssets/` 只保存资源加载、访问、缓存和释放代码，不保存 PNG 或其它资源文件。
- Domain 和 Application 使用资源 ID 或配置对象，不直接依赖硬编码的 Cocos 资源路径；具体路径解析由 Infrastructure 负责。

### 通关重置与永久进度

当前通关结算规则：

- 通关后，所有英雄等级重置为初始等级；英雄等级属于本轮成长数据。
- 通关后，清空常规货币 `currency` 的余额。
- `treasure` 掉落对应的宝物可以提供永久加成；已获得的宝物、解锁状态和永久加成在通关后保留。
- `drops.type` 只允许 `currency` 和 `treasure`，不使用第三种掉落类型表达上述两类之外的经济状态。
- Boss 配置只描述掉落内容和概率；重置、保留和存档行为由 Domain 的 Progression/Economy/Save 相关模块统一处理。

### 当前架构红线

1. 不把 Cocos `Node` 当作游戏数据模型。
2. 不把所有逻辑塞进一个 `GameManager` 或 `GameRoot`。
3. 不为点击和挂机分别实现两套战斗规则。
4. 不让 View 直接修改金币、Boss 血量、升级和关卡状态。
5. 不让每个模块自行读写存档。
6. 不让像素贴图资源依赖 Domain 规则。
7. 不为了模仿大型项目而过早拆分过多系统。
