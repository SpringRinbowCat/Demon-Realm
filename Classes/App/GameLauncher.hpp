#pragma once

#include "cocos2d.h"

namespace DemonRealm
{

/// Cocos 应用启动入口。
///
/// 职责：创建窗口和渲染视图、设置设计分辨率与帧率、装配首个场景，并响应桌面端
/// 的前后台切换。该类只做启动和平台生命周期适配，不承载玩法规则；业务系统的组合
/// 由后续的 `GameRoot` 负责。
///
/// 线程要求：只能在主线程创建和使用。
class GameLauncher : public cocos2d::Application
{
public:
    GameLauncher();
    ~GameLauncher() override;

    /// 设置渲染上下文属性，由引擎在创建渲染视图前调用。
    void initGLContextAttrs() override;

    /// 启动完成回调：装配窗口、设计分辨率、帧率和首个场景。
    /// 返回值：启动成功返回 true；渲染视图或首个场景创建失败返回 false。
    bool applicationDidFinishLaunching() override;

    /// 应用进入后台：停止动画驱动，避免后台继续推进表现。
    void applicationDidEnterBackground() override;

    /// 应用回到前台：恢复动画驱动。
    void applicationWillEnterForeground() override;
};

}  // namespace DemonRealm
