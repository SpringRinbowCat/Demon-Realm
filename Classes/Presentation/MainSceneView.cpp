#include "Presentation/MainSceneView.hpp"

#include <new>

#include "cocos2d.h"

#include "Presentation/EnterGameView.hpp"

namespace DemonRealm
{
namespace
{

/// 进入游戏页面的渲染层级。
const int kEnterGamePageZOrder = 0;

}  // namespace

MainSceneView* MainSceneView::create()
{
    MainSceneView* scene = new (std::nothrow) MainSceneView();
    if (scene == nullptr)
    {
        cocos2d::log("[MainSceneView] failed to allocate scene");
        return nullptr;
    }

    if (!scene->init())
    {
        // 初始化失败的对象还没有进入自动释放池，这里直接释放，避免泄漏。
        delete scene;
        return nullptr;
    }

    scene->autorelease();
    return scene;
}

bool MainSceneView::init()
{
    if (!cocos2d::Scene::init())
    {
        return false;
    }

    return _setUpEnterGamePage();
}

bool MainSceneView::_setUpEnterGamePage()
{
    EnterGameView* enterGameView = EnterGameView::create();
    if (enterGameView == nullptr)
    {
        cocos2d::log("[MainSceneView] failed to create enter game page");
        return false;
    }

    enterGameView->setOnEnterGameRequested([this]() { _onEnterGameRequested(); });
    addChild(enterGameView, kEnterGamePageZOrder);
    _enterGameView = enterGameView;
    return true;
}

void MainSceneView::_onEnterGameRequested()
{
    // 页面切换属于场景宿主的职责；目标页面尚未实现，这里先记录请求。
    cocos2d::log("[MainSceneView] enter game requested; target page is not implemented yet");
}

}  // namespace DemonRealm
