#pragma once

#include <string>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

namespace DemonRealm
{

/// 像素风格控件的创建入口。
///
/// 职责：统一像素贴图的采样方式、字体与资源目录，避免每个视图各写一份精灵和文字的
/// 创建代码。只负责"造出节点"，布局与业务含义由各视图自己决定。
///
/// 线程要求：只能在主线程调用。
namespace PixelWidgets
{

/// 运行时贴图目录，均相对 Resources 根目录。
extern const char* const kBackgroundDirectory;
extern const char* const kBossDirectory;
extern const char* const kHeroDirectory;
extern const char* const kUiDirectory;

/// 创建像素贴图精灵，并按最近邻采样避免像素格被插值模糊。
/// 参数 path：相对 Resources 根目录的贴图路径。
/// 返回值：加载失败时返回 nullptr。
cocos2d::Sprite* createSprite(const std::string& path);

/// 创建单行文字节点。
/// 参数 text：文字内容。
/// 参数 fontSize：字号。
/// 参数 color：文字颜色。
/// 返回值：创建失败时返回 nullptr。
cocos2d::Label* createLabel(const std::string& text, float fontSize, const cocos2d::Color3B& color);

/// 创建按宽度自动换行的多行文字节点。
///
/// 中文没有空格，需要允许在任意字符处断行，否则整段文字会溢出到卡片之外。
///
/// 参数 text：文字内容。
/// 参数 fontSize：字号。
/// 参数 color：文字颜色。
/// 参数 width：换行宽度。
/// 参数 alignment：水平对齐方式。
/// 返回值：创建失败时返回 nullptr。
cocos2d::Label* createWrappedLabel(const std::string& text,
                                   float fontSize,
                                   const cocos2d::Color3B& color,
                                   float width,
                                   cocos2d::TextHAlignment alignment);

/// 创建像素贴图按钮，并按最近邻采样处理贴图。
///
/// 按钮文字由调用方作为子节点添加，保持与项目其他按钮一致的字体处理方式。
///
/// 参数 normalImagePath：常态贴图路径。
/// 参数 pressedImagePath：按下态贴图路径。
/// 返回值：创建失败时返回 nullptr。
cocos2d::ui::Button* createButton(const std::string& normalImagePath, const std::string& pressedImagePath);

}  // namespace PixelWidgets

}  // namespace DemonRealm
