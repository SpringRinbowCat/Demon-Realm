#include "Presentation/PixelWidgets.hpp"

namespace DemonRealm
{
namespace PixelWidgets
{
namespace
{

/// 文字使用的系统字体名；项目暂未接入像素字体资源，中文依赖系统字体回退。
const char* const kFontName = "Arial";

/// 自动换行时不限制高度，交给 Label 按内容撑开。
const float kAutoHeight = 0.0F;

/// 对精灵纹理设置最近邻采样。
void applyPixelTextureFilter(cocos2d::Sprite* sprite)
{
    if (sprite == nullptr)
    {
        return;
    }

    cocos2d::Texture2D* texture = sprite->getTexture();
    if (texture != nullptr)
    {
        texture->setAliasTexParameters();
    }
}

}  // namespace

const char* const kBackgroundDirectory = "Textures/Pixel/Backgrounds/";
const char* const kBossDirectory = "Textures/Pixel/Bosses/";
const char* const kHeroDirectory = "Textures/Pixel/Heroes/";
const char* const kUiDirectory = "Textures/Pixel/UI/";

cocos2d::Sprite* createSprite(const std::string& path)
{
    cocos2d::Sprite* sprite = cocos2d::Sprite::create(path);
    if (sprite == nullptr)
    {
        cocos2d::log("[PixelWidgets] failed to load image: %s", path.c_str());
        return nullptr;
    }

    applyPixelTextureFilter(sprite);
    return sprite;
}

cocos2d::Label* createLabel(const std::string& text, float fontSize, const cocos2d::Color3B& color)
{
    cocos2d::Label* label = cocos2d::Label::createWithSystemFont(text, kFontName, fontSize);
    if (label == nullptr)
    {
        cocos2d::log("[PixelWidgets] failed to create label: %s", text.c_str());
        return nullptr;
    }

    label->setTextColor(cocos2d::Color4B(color));
    return label;
}

cocos2d::Label* createWrappedLabel(const std::string& text,
                                   float fontSize,
                                   const cocos2d::Color3B& color,
                                   float width,
                                   cocos2d::TextHAlignment alignment)
{
    cocos2d::Label* label = cocos2d::Label::createWithSystemFont(text,
                                                                kFontName,
                                                                fontSize,
                                                                cocos2d::Size(width, kAutoHeight),
                                                                alignment);
    if (label == nullptr)
    {
        cocos2d::log("[PixelWidgets] failed to create wrapped label: %s", text.c_str());
        return nullptr;
    }

    // 中文之间没有空格，必须允许在任意字符处断行，否则整段文字会溢出容器。
    label->setLineBreakWithoutSpace(true);
    label->setTextColor(cocos2d::Color4B(color));
    return label;
}

cocos2d::ui::Button* createButton(const std::string& normalImagePath, const std::string& pressedImagePath)
{
    cocos2d::ui::Button* button = cocos2d::ui::Button::create(normalImagePath, pressedImagePath);
    if (button == nullptr)
    {
        cocos2d::log("[PixelWidgets] failed to create button: %s", normalImagePath.c_str());
        return nullptr;
    }

    // 贴图按原始像素格显示，不做九宫格拉伸，避免像素密度被破坏。
    button->setScale9Enabled(false);
    applyPixelTextureFilter(button->getRendererNormal());
    applyPixelTextureFilter(button->getRendererClicked());
    return button;
}

}  // namespace PixelWidgets

}  // namespace DemonRealm
