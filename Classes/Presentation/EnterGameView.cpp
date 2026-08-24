#include "Presentation/EnterGameView.hpp"

#include <new>

#include "cocos2d.h"

namespace DemonRealm
{
namespace
{

/// 页面背景图资源路径，相对 Resources 根目录。
/// 进入游戏页面使用独立背景，不复用战斗场景的背景。
/// 资源文件名按项目规范使用中文加下划线，源文件以 UTF-8 保存。
const char* const kBackgroundImagePath = "Textures/Pixel/Backgrounds/背景_进入游戏界面.png";

/// 进入游戏按钮常态贴图路径，相对 Resources 根目录。
const char* const kEnterGameButtonNormalImagePath = "Textures/Pixel/UI/按钮_进入游戏_常态.png";

/// 进入游戏按钮按下态贴图路径，相对 Resources 根目录。
const char* const kEnterGameButtonPressedImagePath = "Textures/Pixel/UI/按钮_进入游戏_按下.png";

/// 进入游戏按钮文字。
const char* const kEnterGameButtonTitle = "进入游戏";

/// 按钮文字使用的系统字体名；项目暂未接入像素字体资源。
const char* const kEnterGameButtonFontName = "Arial";

/// 按钮文字字号，与 300x90 的按钮贴图尺寸匹配。
const float kEnterGameButtonFontSize = 34.0F;

/// 取中点使用的比例系数。
const float kCenterFactor = 0.5F;

/// 进入游戏按钮中心在可见区域高度上的比例位置，落在背景下半纯色区。
const float kEnterGameButtonCenterHeightRatio = 0.26F;

/// 背景渲染层级。
const int kBackgroundZOrder = 0;

/// 进入游戏按钮渲染层级。
const int kEnterGameButtonZOrder = 10;

/// 按钮文字颜色，取自背景调色板中的深色，保证在橙色按钮上的可读对比度。
const cocos2d::Color3B kEnterGameButtonTitleColor(24, 3, 8);

/// 像素贴图统一使用最近邻采样，避免线性过滤模糊像素格。
/// 参数 sprite：目标精灵；传入 nullptr 时直接返回。
void applyPixelTextureFilter(cocos2d::Sprite* sprite)
{
    if (sprite == nullptr)
    {
        return;
    }

    cocos2d::Texture2D* texture = sprite->getTexture();
    if (texture == nullptr)
    {
        return;
    }

    texture->setAliasTexParameters();
}

/// 按比例计算可见区域内的位置。
/// 参数 widthRatio：相对可见区域宽度的比例。
/// 参数 heightRatio：相对可见区域高度的比例。
/// 返回值：可见区域内的坐标；Director 不可用时返回原点。
cocos2d::Vec2 visiblePosition(float widthRatio, float heightRatio)
{
    const cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return cocos2d::Vec2::ZERO;
    }

    const cocos2d::Size visibleSize = director->getVisibleSize();
    const cocos2d::Vec2 visibleOrigin = director->getVisibleOrigin();
    return cocos2d::Vec2(visibleOrigin.x + visibleSize.width * widthRatio,
                         visibleOrigin.y + visibleSize.height * heightRatio);
}

}  // namespace

EnterGameView* EnterGameView::create()
{
    EnterGameView* view = new (std::nothrow) EnterGameView();
    if (view == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to allocate view");
        return nullptr;
    }

    if (!view->init())
    {
        // 初始化失败的对象还没有进入自动释放池，这里直接释放，避免泄漏。
        delete view;
        return nullptr;
    }

    view->autorelease();
    return view;
}

bool EnterGameView::init()
{
    if (!cocos2d::Node::init())
    {
        return false;
    }

    if (!_setUpBackground())
    {
        return false;
    }

    return _setUpEnterGameButton();
}

void EnterGameView::setOnEnterGameRequested(const EnterGameRequestedCallback& callback)
{
    _onEnterGameRequested = callback;
}

bool EnterGameView::_setUpBackground()
{
    cocos2d::Sprite* background = cocos2d::Sprite::create(kBackgroundImagePath);
    if (background == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to load background image: %s", kBackgroundImagePath);
        return false;
    }

    applyPixelTextureFilter(background);
    // 背景按设计分辨率 1:1 呈现，不做运行时缩放，保持既有像素格密度。
    background->setPosition(visiblePosition(kCenterFactor, kCenterFactor));
    addChild(background, kBackgroundZOrder);
    _backgroundSprite = background;
    return true;
}

bool EnterGameView::_setUpEnterGameButton()
{
    cocos2d::MenuItemSprite* buttonItem = _createEnterGameButtonItem();
    if (buttonItem == nullptr)
    {
        return false;
    }

    cocos2d::Menu* menu = cocos2d::Menu::createWithItem(buttonItem);
    if (menu == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to create enter game menu");
        return false;
    }

    menu->setPosition(visiblePosition(kCenterFactor, kEnterGameButtonCenterHeightRatio));
    addChild(menu, kEnterGameButtonZOrder);
    _enterGameMenu = menu;
    return true;
}

cocos2d::MenuItemSprite* EnterGameView::_createEnterGameButtonItem()
{
    cocos2d::Sprite* normalSprite = cocos2d::Sprite::create(kEnterGameButtonNormalImagePath);
    cocos2d::Sprite* pressedSprite = cocos2d::Sprite::create(kEnterGameButtonPressedImagePath);
    if (normalSprite == nullptr || pressedSprite == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to load enter game button textures");
        return nullptr;
    }

    applyPixelTextureFilter(normalSprite);
    applyPixelTextureFilter(pressedSprite);

    cocos2d::MenuItemSprite* buttonItem = cocos2d::MenuItemSprite::create(
        normalSprite,
        pressedSprite,
        [this](cocos2d::Ref* sender) {
            static_cast<void>(sender);
            _onEnterGameButtonClicked();
        });
    if (buttonItem == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to create enter game button item");
        return nullptr;
    }

    cocos2d::Label* title = cocos2d::Label::createWithSystemFont(kEnterGameButtonTitle,
                                                                 kEnterGameButtonFontName,
                                                                 kEnterGameButtonFontSize);
    if (title == nullptr)
    {
        cocos2d::log("[EnterGameView] failed to create enter game button title");
        return nullptr;
    }

    const cocos2d::Size buttonSize = buttonItem->getContentSize();
    title->setTextColor(cocos2d::Color4B(kEnterGameButtonTitleColor));
    title->setPosition(cocos2d::Vec2(buttonSize.width * kCenterFactor, buttonSize.height * kCenterFactor));
    buttonItem->addChild(title);
    return buttonItem;
}

void EnterGameView::_onEnterGameButtonClicked()
{
    // 当前为空实现：只记录点击，真正的场景跳转在后续步骤由上层接入。
    cocos2d::log("[EnterGameView] enter game button clicked; scene transition is not implemented yet");

    if (_onEnterGameRequested)
    {
        _onEnterGameRequested();
    }
}

}  // namespace DemonRealm
