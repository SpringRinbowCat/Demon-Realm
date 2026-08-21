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
- 当前 Classes 和部分 Resources 目录已建立，但 C++ 业务实现仍在逐步建设中；模块 README 中标注的目标结构不等同于已完成代码。

## 仓库入口

```text
Demon-Realm/
├── Classes/                 # C++ 代码模块，见 Classes/README.md
├── Resources/               # 运行时配置与资源，见 Resources/README.md
├── ArtSource/               # 本地美术源文件，不参与运行时打包
└── README.md                # 项目入口和模块职责说明
```
