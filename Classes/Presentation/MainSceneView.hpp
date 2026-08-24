#pragma once

#include "cocos2d.h"

namespace DemonRealm
{

class EnterGameView;

/// 主场景视图，单场景宿主的表现根节点。
///
/// 职责：作为唯一场景的根节点挂载当前页面，并承担页面之间的切换入口。场景本身只
/// 组织页面和转发页面回传的请求，不持有业务状态，也不计算业务规则。
///
/// 线程要求：只能在主线程创建、访问和销毁。
class MainSceneView : public cocos2d::Scene
{
public:
    /// 创建主场景。
    /// 返回值：已加入自动释放池的场景对象；分配或初始化失败时返回 nullptr。
    static MainSceneView* create();

    /// 初始化场景内容，挂载首个页面。
    /// 返回值：初始化成功返回 true；基类初始化或页面创建失败返回 false。
    bool init() override;

private:
    /// 挂载进入游戏页面，并接入页面回传的进入游戏请求。
    /// 返回值：页面创建并挂载成功返回 true。
    bool _setUpEnterGamePage();

    /// 进入游戏请求响应；页面跳转在后续步骤实现，当前只输出日志。
    void _onEnterGameRequested();

    /// 进入游戏页面视图；生命周期由节点树持有，这里只保存非拥有引用。
    EnterGameView* _enterGameView = nullptr;
};

}  // namespace DemonRealm
