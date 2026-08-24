# 魔域（Demon-Realm）

本项目是一个使用 Cocos、C++ 和像素格贴图构建的 Windows/macOS 点击与挂机游戏。项目内容按代码模块和运行时资源分为 `Classes/` 与 `Resources/` 两个主要目录。

## Classes：代码模块

`Classes/` 是 C++ 代码根目录，负责：

- 游戏状态和领域规则
- 点击、挂机、升级和关卡等业务用例编排
- Cocos 场景、UI、输入和动画的表现适配
- 配置、存档、时间、平台和像素资源等外部系统适配
- 跨模块的类型化事件

Classes 不保存 PNG、JSON 或其它运行时资源。详细的分层职责、目标目录、数据流和架构红线见 [Classes/README.md](Classes/README.md)。

## Resources：运行时资源模块

`Resources/` 保存游戏运行时需要读取或打包的内容，负责：

- Boss、英雄、关卡、升级和奖励等 JSON 配置
- 背景、Boss、人物、UI 和特效等像素贴图
- 动画、音频、字体和 Shader 资源
- 资源尺寸、像素密度、透明边界、命名和来源授权规范

Resources 不保存 C++ 代码，也不承载伤害、金币、升级、关卡或存档规则的实现。详细的资源目录、配置契约、像素图片规范和来源记录见 [Resources/README.md](Resources/README.md)。

## 两个模块的边界

- `Classes` 通过 Infrastructure/Config、PixelAssets、Save 等明确接口读取或使用 `Resources`，不直接把资源文件当作业务模型。
- `Resources` 只描述运行时数据和表现资源，不依赖 C++ 业务实现。
- `ArtSource/` 保存美术编辑源文件和授权记录，不参与运行时资源打包。
- 当前已接入 cocos2d-x 4.0，macOS 可构建运行：应用入口、主场景和进入游戏页面（背景图与进入游戏按钮）已打通，按钮点击暂为日志空实现，页面跳转未实现。
- 战斗、经济、关卡、升级、挂机和存档等业务系统尚未实现；模块 README 中标注的目标结构不等同于已完成代码。

## 构建与运行

引擎不纳入版本库：`cocos2d/` 是本地检出的 cocos2d-x 4.0，已在 `.gitignore` 中忽略。首次准备环境（macOS）：

```bash
# 1. 检出引擎
git clone --depth 1 --branch cocos2d-x-4.0 https://github.com/cocos2d/cocos2d-x.git cocos2d

# 2. 获取引擎第三方依赖，版本号取自 cocos2d/external/config.json
curl -L -o /tmp/cocos2d-3rd-party.zip \
  https://github.com/cocos2d/cocos2d-x-3rd-party-libs-bin/archive/refs/tags/metal-support-22.zip
unzip -q /tmp/cocos2d-3rd-party.zip -d /tmp/cocos2d-deps
cp -R /tmp/cocos2d-deps/cocos2d-x-3rd-party-libs-bin-metal-support-22/. cocos2d/external/

# 3. 应用引擎补丁
git -C cocos2d apply ../patches/cocos2d-x-4.0/0001-fix-iconv-argument-type-on-modern-macos-sdk.patch

# 4. 配置并构建（Ninja 单配置生成器）
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 5. 运行
./build/bin/DemonRealm/DemonRealm.app/Contents/MacOS/DemonRealm
```

也可以用 Xcode 生成器（多配置，产物多一层配置目录）：

```bash
cmake -S . -B build -G Xcode
cmake --build build --config Debug --parallel
./build/bin/DemonRealm/Debug/DemonRealm.app/Contents/MacOS/DemonRealm
```

IDE 内的 CMake Tools 直接配置构建即可，不需要额外传参数：目标架构已固定在 `CMakeLists.txt` 中。

构建约束与已知适配：

- **架构**：cocos2d-x 4.0 的 macOS 预编译库只有 `i386/x86_64`，没有 `arm64`。因此 `CMakeLists.txt` 在 `project()` 之前把 `CMAKE_OSX_ARCHITECTURES` 默认设为 `x86_64`，Apple Silicon 上运行依赖 Rosetta。不要在 Apple Silicon 上按原生 `arm64` 配置，否则所有预编译库会被链接器忽略并报 `symbol(s) not found for architecture arm64`。换成含 `arm64` 的第三方库后，配置时显式传 `-DCMAKE_OSX_ARCHITECTURES=arm64` 覆盖即可。
- **工具链兼容**：`cmake/EngineToolchainCompat.hpp` 通过编译选项 `-include` 只对引擎目标补齐新版 libc++ 已移除的传递包含；本项目 `Classes/` 代码不依赖该兼容头，仍须显式包含自己用到的头文件。
- **引擎补丁**：`patches/cocos2d-x-4.0/` 保存对引擎源码的最小修改（当前只有新版 macOS SDK 的 `iconv` 参数类型修复）。引擎目录被忽略，补丁必须留在版本库里才能复现构建。
- **引擎维护状态**：cocos2d-x 自 2019 年起不再更新，上述补丁与兼容措施属于对旧引擎的适配成本。

新增或移除 `Classes/` 下的源文件时，必须同步更新根目录 `CMakeLists.txt` 中的 `GAME_HEADER` 与 `GAME_SOURCE` 列表。

## 仓库入口

```text
Demon-Realm/
├── Classes/                 # C++ 代码模块，见 Classes/README.md
├── Resources/               # 运行时配置与资源，见 Resources/README.md
├── proj.ios_mac/            # macOS 平台入口与打包配置
├── proj.win32/              # Windows 平台入口与打包配置
├── cmake/                   # 构建期辅助头与工具链兼容文件
├── patches/                 # 对 cocos2d-x 引擎的最小修改补丁
├── CMakeLists.txt           # 项目构建脚本
├── ArtSource/               # 本地美术源文件，不参与运行时打包（已忽略）
├── cocos2d/                 # 本地检出的 cocos2d-x 4.0 引擎（已忽略）
└── README.md                # 项目入口和模块职责说明
```
