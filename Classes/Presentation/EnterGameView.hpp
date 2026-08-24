#pragma once

#include <functional>

#include "cocos2d.h"

namespace DemonRealm
{

/// 进入游戏页面视图。
///
/// 职责：在单场景宿主内铺设进入游戏页面的背景图和进入游戏按钮，并把按钮点击
/// 输入回传给上层。该视图只负责表现和输入回传，不持有业务状态，也不计算战斗、
/// 金币、升级或关卡规则。
///
/// 使用场景：由场景装配层创建后挂载到主场景根节点。进入游戏后的实际跳转行为由
/// 上层通过 setOnEnterGameRequested 注入；未注入时按钮点击只输出日志。
///
/// 线程要求：只能在主线程创建、访问和销毁。
class EnterGameView : public cocos2d::Node
{
public:
    /// 进入游戏请求回调类型；由上层决定实际跳转目标和跳转方式。
    using EnterGameRequestedCallback = std::function<void()>;

    /// 创建进入游戏页面视图。
    /// 返回值：已加入自动释放池的视图对象；分配或初始化失败时返回 nullptr。
    static EnterGameView* create();

    /// 初始化页面内容，依次铺设背景图和进入游戏按钮。
    /// 返回值：初始化成功返回 true；基类初始化失败或页面资源缺失时返回 false。
    bool init() override;

    /// 设置进入游戏请求回调。
    /// 参数 callback：上层注入的回调；传入空回调表示只保留日志行为。
    void setOnEnterGameRequested(const EnterGameRequestedCallback& callback);

private:
    /// 铺设页面背景图，按可见区域居中，不做运行时非整数缩放。
    /// 返回值：背景加载并挂载成功返回 true。
    bool _setUpBackground();

    /// 创建进入游戏按钮、按钮文字，并把按钮挂载到页面上。
    /// 返回值：按钮创建并挂载成功返回 true。
    bool _setUpEnterGameButton();

    /// 创建进入游戏按钮项，包含常态贴图、按下态贴图、文字和点击回调。
    /// 返回值：创建成功返回按钮项；贴图或文字缺失时返回 nullptr。
    cocos2d::MenuItemSprite* _createEnterGameButtonItem();

    /// 进入游戏按钮点击响应；当前只输出日志，并把请求回传给上层回调。
    void _onEnterGameButtonClicked();

    /// 页面背景精灵；生命周期由节点树持有，这里只保存非拥有引用。
    cocos2d::Sprite* _backgroundSprite = nullptr;

    /// 进入游戏按钮所在菜单；生命周期由节点树持有，这里只保存非拥有引用。
    cocos2d::Menu* _enterGameMenu = nullptr;

    /// 上层注入的进入游戏请求回调；未设置时点击只输出日志。
    EnterGameRequestedCallback _onEnterGameRequested;
};

}  // namespace DemonRealm
