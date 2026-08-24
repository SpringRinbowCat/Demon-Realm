#include "App/GameLauncher.hpp"

#include "cocos2d.h"

#include "Presentation/MainSceneView.hpp"

namespace DemonRealm
{
namespace
{

/// 设计分辨率宽度，与像素资源的 540x960 竖屏基准一致。
const float kDesignResolutionWidth = 540.0F;

/// 设计分辨率高度，与像素资源的 540x960 竖屏基准一致。
const float kDesignResolutionHeight = 960.0F;

/// 目标帧率。
const double kTargetFramesPerSecond = 60.0;

/// 桌面端窗口标题。
const char* const kWindowTitle = "Demon-Realm";

/// 渲染上下文每个颜色通道的位数。
const int kColorChannelBits = 8;

/// 渲染上下文深度缓冲位数。
const int kDepthBufferBits = 24;

/// 渲染上下文模板缓冲位数。
const int kStencilBufferBits = 8;

/// 多重采样数量；像素风格贴图不使用多重采样，避免边缘被抗锯齿。
const int kMultisamplingCount = 0;

}  // namespace

GameLauncher::GameLauncher() = default;

GameLauncher::~GameLauncher() = default;

void GameLauncher::initGLContextAttrs()
{
    // GLContextAttrs 与 ResolutionPolicy 由引擎声明在全局作用域，不在 cocos2d 命名空间内。
    ::GLContextAttrs glContextAttrs = {kColorChannelBits,
                                       kColorChannelBits,
                                       kColorChannelBits,
                                       kColorChannelBits,
                                       kDepthBufferBits,
                                       kStencilBufferBits,
                                       kMultisamplingCount};
    cocos2d::GLView::setGLContextAttrs(glContextAttrs);
}

bool GameLauncher::applicationDidFinishLaunching()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return false;
    }

    cocos2d::GLView* glView = director->getOpenGLView();
    if (glView == nullptr)
    {
        glView = cocos2d::GLViewImpl::createWithRect(
            kWindowTitle,
            cocos2d::Rect(0.0F, 0.0F, kDesignResolutionWidth, kDesignResolutionHeight));
        if (glView == nullptr)
        {
            cocos2d::log("[GameLauncher] failed to create render view");
            return false;
        }

        director->setOpenGLView(glView);
    }

    // 设计分辨率与像素资源基准一致，窗口按 1:1 呈现，避免非整数缩放破坏像素格。
    glView->setDesignResolutionSize(kDesignResolutionWidth,
                                    kDesignResolutionHeight,
                                    ::ResolutionPolicy::SHOW_ALL);
    director->setAnimationInterval(1.0 / kTargetFramesPerSecond);

    MainSceneView* scene = MainSceneView::create();
    if (scene == nullptr)
    {
        cocos2d::log("[GameLauncher] failed to create main scene");
        return false;
    }

    director->runWithScene(scene);
    return true;
}

void GameLauncher::applicationDidEnterBackground()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return;
    }

    director->stopAnimation();
}

void GameLauncher::applicationWillEnterForeground()
{
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return;
    }

    director->startAnimation();
}

}  // namespace DemonRealm
