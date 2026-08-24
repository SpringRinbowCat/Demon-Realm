# Classes 模块

> 本文档中的路径均相对于 `Classes/` 目录；仓库根目录说明见 [../README.md](../README.md)，运行时资源说明见 [../Resources/README.md](../Resources/README.md)。

## 模块范围与当前状态

`Classes/` 是项目 C++ 代码根目录，负责游戏状态、业务规则、用例编排、Cocos 表现适配、存档和其它外部系统适配。

当前目录已经按目标架构建立，实现刚刚起步。已实现：

- `App/GameLauncher`：Cocos 应用启动入口，设置渲染上下文、设计分辨率、帧率并装配首个场景。
- `Presentation/MainSceneView`：单场景宿主的表现根节点，挂载当前页面并接收页面回传的请求。
- `Presentation/EnterGameView`：进入游戏页面，包含背景图和进入游戏按钮。

其余目录仍只有 `.gitkeep`，尚未实现 `GameRoot`、Controller、System 或 Service。下文中未标注为已实现的类名、数据流和目录职责属于目标设计，不代表现有代码已经完成。

项目已接入 cocos2d-x 4.0，macOS 上可以构建运行；平台入口在仓库根目录的 `proj.ios_mac/mac/main.cpp` 和 `proj.win32/main.cpp`，只负责创建 `GameLauncher` 并进入引擎主循环。构建步骤、架构限制和引擎补丁见 [../README.md](../README.md)。新增或移除源文件时必须同步更新根目录 `CMakeLists.txt` 的源文件列表。

代码约定：业务代码放在 `DemonRealm` 命名空间中，文件按 `.hpp` / `.cpp` 拆分，命名和边界规则见 `.kiro/skills/code-architecture-standards`。

Classes 不保存 PNG、JSON 或其它运行时资源。资源由 [Resources 模块](../Resources/README.md) 管理，Classes 只能通过配置、资源仓储或明确的 Infrastructure 接口访问它们。

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

### 挂机攻击

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

`IdleSystem` 只负责时间和批量结果，不得复制另一套伤害、金币或 Boss 结算规则。

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

## 已实现页面：进入游戏页面

`Presentation/EnterGameView` 是当前唯一已实现的视图，按三步交付：

1. **背景图**：加载页面专用背景 `Textures/Pixel/Backgrounds/背景_进入游戏界面.png`，在可见区域居中，按设计分辨率 1:1 呈现，不做运行时缩放，并对贴图设置最近邻采样。页面背景与战斗场景背景相互独立，不共用同一张贴图。
2. **进入游戏按钮**：用 `Textures/Pixel/UI/按钮_进入游戏_常态.png` 和 `按钮_进入游戏_按下.png` 组成 `MenuItemSprite`，挂在 `Menu` 上接收点击；按钮文字暂用系统字体绘制，接入像素字体后再替换。
3. **点击响应**：`_onEnterGameButtonClicked()` 当前只输出日志，并调用上层通过 `setOnEnterGameRequested()` 注入的回调；视图本身不决定跳转目标。

边界约束：

- 视图继承 `cocos2d::Node`，只做表现和输入回传，不持有业务状态，也不计算战斗、金币、升级或关卡规则。
- 按钮点击不直接切换场景；视图只回传“请求进入游戏”这一事实，真实跳转由上层实现。
- 视图内不启动线程、不持有异步句柄，只能在主线程创建、访问和销毁。
- 资源路径集中在实现文件的常量中，业务层不硬编码 Cocos 资源路径。
- 背景精灵和按钮菜单的生命周期由节点树持有，视图只保存非拥有引用。

页面挂载与请求回传链路：

```text
GameLauncher（设置窗口、设计分辨率、帧率）
    ↓ runWithScene
MainSceneView（单场景宿主）
    ↓ addChild + setOnEnterGameRequested
EnterGameView（背景图 + 进入游戏按钮）
    ↓ 点击
EnterGameView 输出日志并回传请求
    ↓
MainSceneView 记录请求（页面跳转待实现）
```

待补齐项：`GameRoot` 尚未实现，目前由 `GameLauncher` 直接装配场景；进入游戏后的目标页面和真正的页面切换也尚未实现。

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
│   ├── State/                   # 玩家、Boss、关卡、金币和升级状态
│   ├── Combat/                  # 点击、挂机、离线攻击和伤害结算
│   ├── Economy/                 # 金币收入、消费和经济规则
│   ├── Progression/             # 关卡推进、解锁和通关奖励
│   └── Idle/                    # 在线挂机、离线时间和离线收益
├── Application/
│   ├── CombatController          # 接收攻击输入并调用 CombatSystem
│   ├── UpgradeController         # 处理升级购买请求
│   ├── StageController           # 处理关卡进入、切换和重试
│   └── IdleController            # 处理自动战斗和离线收益领取
├── Presentation/                 # Cocos 场景、UI、输入和动画表现
│   ├── EnterGameView             # 进入游戏页面背景与进入游戏按钮（已实现）
│   ├── MainSceneView             # 单场景表现根节点（已实现）
│   ├── BossView                  # Boss 贴图、血条和受击表现
│   ├── GoldView                  # 金币数量和奖励表现
│   ├── UpgradeView               # 升级项目、等级和价格界面
│   ├── StageView                 # 关卡和通关状态界面
│   ├── PixelSpriteView           # 像素 Sprite 和帧动画表现
│   └── GameViewAdapter           # 场景节点与各 View 的装配
├── Infrastructure/
│   ├── Save/                     # 存档、加载和版本迁移
│   ├── Config/                   # Boss、关卡和升级配置读取
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

新增代码时，先确认职责归属、依赖方向、所有权、生命周期、线程边界和错误处理，再扩展对应目录；不要用临时全局状态、跨层指针或 View 特判绕过架构边界。
