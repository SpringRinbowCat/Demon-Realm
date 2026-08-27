#include "Presentation/HeroCardView.hpp"

#include <new>
#include <string>
#include <vector>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Presentation/Format/NumberFormatter.hpp"
#include "Presentation/Format/SkillDescriptionFormatter.hpp"
#include "Presentation/PixelWidgets.hpp"

namespace DemonRealm
{
namespace
{

/// 卡片贴图文件名。
const char* const kHeroCardPanelImageFile = "面板_英雄卡片.png";
const char* const kUpgradeButtonNormalImageFile = "按钮_升级_常态.png";
const char* const kUpgradeButtonPressedImageFile = "按钮_升级_按下.png";
const char* const kGoldSmallIconImageFile = "图标_金币_小.png";

/// 卡片尺寸，与 面板_英雄卡片.png 一致。
const float kCardWidth = 520.0F;
const float kCardHeight = 110.0F;

/// 取中点使用的比例系数。
const float kCenterFactor = 0.5F;

/// 折叠区布局；卡片原点在左上角，向下为负。
const float kPortraitCenterX = 60.0F;
const float kNameLeftX = 120.0F;
const float kLevelLeftX = 300.0F;
const float kFirstLineY = -22.0F;
const float kLineSpacing = 23.0F;
const float kNameFontSize = 20.0F;
const float kStatFontSize = 16.0F;

/// 展开区布局。
const float kDetailSidePadding = 20.0F;
const float kDetailTopPadding = 12.0F;
const float kDetailBottomPadding = 14.0F;
const float kDetailContentWidth = kCardWidth - kDetailSidePadding * 2.0F;

/// 升级块布局。
const float kUpgradeButtonWidth = 180.0F;
const float kUpgradeButtonHeight = 50.0F;
const float kUpgradeButtonFontSize = 18.0F;
const float kUpgradeLevelLeftX = 220.0F;
const float kUpgradeDeltaLeftX = 350.0F;
const float kUpgradeRowGap = 4.0F;
const float kUpgradeCostRowHeight = 30.0F;
const float kUpgradeBlockGap = 10.0F;
const float kUpgradeTextFontSize = 16.0F;

/// 花费行布局。
const float kCostCaptionLeftX = 220.0F;
const float kCostIconCenterX = 292.0F;
const float kCostAmountLeftX = 312.0F;

/// 介绍与技能介绍布局。
const float kDescriptionTopGap = 12.0F;
const float kDescriptionFontSize = 14.0F;
const float kSkillTitleFontSize = 16.0F;
const float kSkillDescriptionFontSize = 13.0F;
const float kSkillEntryGap = 8.0F;
const float kSkillTitleGap = 4.0F;

/// 渲染层级；展开区底色压在文字之下。
const int kDetailBackgroundZOrder = -1;

/// 展开区底色，半透明深色，让卡片与背景拉开层次。
const cocos2d::Color4B kDetailBackgroundColor(24, 3, 8, 216);

/// 文字颜色，取自项目既有调色板。
const cocos2d::Color3B kNameTextColor(255, 220, 61);
const cocos2d::Color3B kStatTextColor(240, 230, 225);
const cocos2d::Color3B kSkillTextColor(255, 173, 28);
const cocos2d::Color3B kLockedTextColor(150, 140, 138);
const cocos2d::Color3B kDescriptionTextColor(214, 205, 200);
const cocos2d::Color3B kUpgradeButtonTextColor(255, 220, 61);

/// 金币不足时的花费颜色。
const cocos2d::Color3B kInsufficientGoldTextColor(224, 64, 52);

/// 判定为点击而非滑动的最大位移，单位为设计坐标像素。
const float kTapMovementTolerance = 12.0F;

/// 数值取不到时的占位文字。
///
/// 例如攻击间隔已经小到 1% 不足最小精度、升级不会再产生效果，此时宁可显示"待定"，
/// 也不显示一个会误导玩家的 0。
const char* const kPlaceholderText = "待定";

/// 花费行的标题文字。
const char* const kCostCaption = "花费";

/// 把等级值组装成展示文字。
std::string formatLevelText(const std::string& level)
{
    return "等级：" + (level.empty() ? std::string(kPlaceholderText) : level);
}

/// 把攻击力变化量组装成展示文字。
std::string formatAttackDeltaText(const std::string& delta)
{
    if (delta.empty())
    {
        return std::string("攻击力 ") + kPlaceholderText;
    }

    return "攻击力 +" + NumberFormatter::formatCompactValue(delta);
}

/// 把攻击间隔变化量组装成展示文字。
std::string formatIntervalDeltaText(const std::string& delta)
{
    if (delta.empty())
    {
        return std::string("攻击间隔 ") + kPlaceholderText;
    }

    return "攻击间隔 -" + NumberFormatter::formatCompactValue(delta) + "秒";
}

/// 把花费金币组装成展示文字。
std::string formatCostText(const std::string& costGoldAmount)
{
    if (costGoldAmount.empty())
    {
        return kPlaceholderText;
    }

    return NumberFormatter::formatIntegerWithGroups(costGoldAmount);
}

/// 按金币是否足够选择花费文字的颜色。
cocos2d::Color3B pickCostColor(bool affordable)
{
    return affordable ? kNameTextColor : kInsufficientGoldTextColor;
}

/// 按当前英雄数值替换技能说明里的数值占位符。
std::string buildSkillDescription(const BattleHeroSnapshot& hero, const BattleHeroSkillSnapshot& skill)
{
    SkillDescriptionFormatter::HeroValues values;
    values.heroId = hero.heroId;
    values.attack = hero.attack;
    values.attackIntervalSeconds = hero.attackIntervalSeconds;
    values.heroLevel = std::to_string(hero.level);
    values.attackLevel = hero.attackUpgrade.level;
    values.attackIntervalLevel = hero.attackIntervalUpgrade.level;
    return SkillDescriptionFormatter::format(skill.description, values);
}

/// 把攻击间隔组装成展示文字，小数位最少 2 位、最多 4 位。
std::string formatIntervalSeconds(const std::string& canonicalSeconds)
{
    return NumberFormatter::formatSecondsWithTrimmedFraction(canonicalSeconds) + "秒";
}

/// 拼接已解锁技能名；没有已解锁技能时返回占位文字。
std::string joinUnlockedSkillNames(const std::vector<std::string>& skillNames)
{
    if (skillNames.empty())
    {
        return "无";
    }

    std::string joined;
    for (const std::string& name : skillNames)
    {
        if (!joined.empty())
        {
            joined += "、";
        }

        joined += name;
    }

    return joined;
}

/// 组装技能标题：未解锁时附带解锁条件。
std::string buildSkillTitle(const BattleHeroSkillSnapshot& skill)
{
    if (skill.unlocked)
    {
        return skill.displayName;
    }

    return skill.displayName + "（等级 " + std::to_string(skill.unlockLevel) + " 解锁）";
}

}  // namespace

const float HeroCardView::kFoldedHeight = kCardHeight;

HeroCardView* HeroCardView::create(const BattleHeroSnapshot& hero)
{
    HeroCardView* view = new (std::nothrow) HeroCardView();
    if (view == nullptr)
    {
        cocos2d::log("[HeroCardView] failed to allocate view");
        return nullptr;
    }

    if (!view->initWithHero(hero))
    {
        // 初始化失败的对象还没有进入自动释放池，这里直接释放，避免泄漏。
        delete view;
        return nullptr;
    }

    view->autorelease();
    return view;
}

bool HeroCardView::initWithHero(const BattleHeroSnapshot& hero)
{
    if (!cocos2d::Node::init())
    {
        return false;
    }

    _hero = hero;
    setContentSize(cocos2d::Size(kCardWidth, kCardHeight));

    return _setUpFoldedArea(hero) && _setUpDetailArea(hero) && _setUpToggleInput();
}

void HeroCardView::updateHero(const BattleHeroSnapshot& hero)
{
    if (_levelLabel != nullptr && hero.level != _hero.level)
    {
        _levelLabel->setString(cocos2d::StringUtils::format("等级：%d", hero.level));
    }

    if (_attackLabel != nullptr && hero.attack != _hero.attack)
    {
        _attackLabel->setString("攻击力：" + NumberFormatter::formatIntegerWithGroups(hero.attack));
    }

    if (_attackIntervalLabel != nullptr && hero.attackIntervalSeconds != _hero.attackIntervalSeconds)
    {
        _attackIntervalLabel->setString("攻击间隔：" + formatIntervalSeconds(hero.attackIntervalSeconds));
    }

    if (_skillsLabel != nullptr && hero.unlockedSkillNames != _hero.unlockedSkillNames)
    {
        _skillsLabel->setString("技能：" + joinUnlockedSkillNames(hero.unlockedSkillNames));
    }

    _hero = hero;
    _rebuildDetailArea();
}

void HeroCardView::_rebuildDetailArea()
{
    // 升级会同时改变等级、增量、花费与技能说明里的数值，换行行数也可能跟着变，
    // 因此整块重建比逐条改文字更可靠。重建只发生在升级或技能成长时，不在每帧路径上。
    if (_detailNode != nullptr)
    {
        removeChild(_detailNode);
        _detailNode = nullptr;
    }

    if (!_setUpDetailArea(_hero))
    {
        cocos2d::log("[HeroCardView] failed to rebuild hero detail area");
        return;
    }

    if (_detailNode != nullptr)
    {
        _detailNode->setVisible(_expanded);
    }
}

void HeroCardView::setExpanded(bool expanded)
{
    _expanded = expanded;
    if (_detailNode != nullptr)
    {
        _detailNode->setVisible(expanded);
    }
}

bool HeroCardView::isExpanded() const
{
    return _expanded;
}

float HeroCardView::getPreferredHeight() const
{
    return _expanded ? kCardHeight + _detailHeight : kCardHeight;
}

void HeroCardView::setOnToggleRequested(const ToggleCallback& callback)
{
    _onToggleRequested = callback;
}

void HeroCardView::setOnUpgradeRequested(const UpgradeCallback& callback)
{
    _onUpgradeRequested = callback;
}

bool HeroCardView::_setUpFoldedArea(const BattleHeroSnapshot& hero)
{
    cocos2d::Sprite* panel = PixelWidgets::createSprite(std::string(PixelWidgets::kUiDirectory)
                                                        + kHeroCardPanelImageFile);
    cocos2d::Sprite* portrait = PixelWidgets::createSprite(std::string(PixelWidgets::kHeroDirectory)
                                                           + hero.cardImageFile);
    if (panel == nullptr || portrait == nullptr)
    {
        return false;
    }

    panel->setPosition(cocos2d::Vec2(kCardWidth * kCenterFactor, -kCardHeight * kCenterFactor));
    addChild(panel);
    portrait->setPosition(cocos2d::Vec2(kPortraitCenterX, -kCardHeight * kCenterFactor));
    addChild(portrait);

    struct FoldedLine
    {
        std::string text;
        float leftX;
        float lineY;
        float fontSize;
        cocos2d::Color3B color;
        cocos2d::Label** target;
    };

    const float nameRowY = kFirstLineY;
    const FoldedLine lines[] = {
        {hero.displayName, kNameLeftX, nameRowY, kNameFontSize, kNameTextColor, nullptr},
        {cocos2d::StringUtils::format("等级：%d", hero.level),
         kLevelLeftX,
         nameRowY,
         kNameFontSize,
         kNameTextColor,
         &_levelLabel},
        {"攻击力：" + NumberFormatter::formatIntegerWithGroups(hero.attack),
         kNameLeftX,
         nameRowY - kLineSpacing,
         kStatFontSize,
         kStatTextColor,
         &_attackLabel},
        {"攻击间隔：" + formatIntervalSeconds(hero.attackIntervalSeconds),
         kNameLeftX,
         nameRowY - kLineSpacing * 2.0F,
         kStatFontSize,
         kStatTextColor,
         &_attackIntervalLabel},
        {"技能：" + joinUnlockedSkillNames(hero.unlockedSkillNames),
         kNameLeftX,
         nameRowY - kLineSpacing * 3.0F,
         kStatFontSize,
         kSkillTextColor,
         &_skillsLabel},
    };

    for (const FoldedLine& line : lines)
    {
        cocos2d::Label* label = PixelWidgets::createLabel(line.text, line.fontSize, line.color);
        if (label == nullptr)
        {
            return false;
        }

        label->setAnchorPoint(cocos2d::Vec2(0.0F, kCenterFactor));
        label->setPosition(cocos2d::Vec2(line.leftX, line.lineY));
        addChild(label);

        if (line.target != nullptr)
        {
            *line.target = label;
        }
    }

    return true;
}

bool HeroCardView::_setUpDetailArea(const BattleHeroSnapshot& hero)
{
    cocos2d::Node* detail = cocos2d::Node::create();
    if (detail == nullptr)
    {
        cocos2d::log("[HeroCardView] failed to create detail node");
        return false;
    }

    detail->setPosition(cocos2d::Vec2(0.0F, -kCardHeight));
    detail->setVisible(false);
    addChild(detail);
    _detailNode = detail;

    UpgradeBlockTexts attackTexts;
    attackTexts.buttonTitle = "升级攻击力";
    attackTexts.level = formatLevelText(hero.attackUpgrade.level);
    attackTexts.delta = formatAttackDeltaText(hero.attackUpgrade.delta);
    attackTexts.cost = formatCostText(hero.attackUpgrade.costGoldAmount);
    attackTexts.affordable = hero.attackUpgrade.affordable;

    UpgradeBlockTexts intervalTexts;
    intervalTexts.buttonTitle = "升级攻击速度";
    intervalTexts.level = formatLevelText(hero.attackIntervalUpgrade.level);
    intervalTexts.delta = formatIntervalDeltaText(hero.attackIntervalUpgrade.delta);
    intervalTexts.cost = formatCostText(hero.attackIntervalUpgrade.costGoldAmount);
    intervalTexts.affordable = hero.attackIntervalUpgrade.affordable;

    float cursorY = -kDetailTopPadding;
    if (!_setUpUpgradeBlock(attackTexts, HeroUpgradeKind::Attack, cursorY))
    {
        return false;
    }

    cursorY -= kUpgradeBlockGap;
    if (!_setUpUpgradeBlock(intervalTexts, HeroUpgradeKind::AttackInterval, cursorY))
    {
        return false;
    }

    if (!_setUpDescriptions(hero, cursorY))
    {
        return false;
    }

    _detailHeight = -cursorY + kDetailBottomPadding;

    // 底色在内容排完后才知道高度，因此最后创建并压到文字下面。
    cocos2d::LayerColor* background =
        cocos2d::LayerColor::create(kDetailBackgroundColor, kCardWidth, _detailHeight);
    if (background == nullptr)
    {
        cocos2d::log("[HeroCardView] failed to create detail background");
        return false;
    }

    background->setPosition(cocos2d::Vec2(0.0F, -_detailHeight));
    detail->addChild(background, kDetailBackgroundZOrder);
    return true;
}

bool HeroCardView::_setUpUpgradeBlock(const UpgradeBlockTexts& texts, HeroUpgradeKind kind, float& cursorY)
{
    cocos2d::ui::Button* button =
        PixelWidgets::createButton(std::string(PixelWidgets::kUiDirectory) + kUpgradeButtonNormalImageFile,
                                   std::string(PixelWidgets::kUiDirectory) + kUpgradeButtonPressedImageFile);
    cocos2d::Label* buttonTitle =
        PixelWidgets::createLabel(texts.buttonTitle, kUpgradeButtonFontSize, kUpgradeButtonTextColor);
    if (button == nullptr || buttonTitle == nullptr)
    {
        return false;
    }

    const float buttonCenterY = cursorY - kUpgradeButtonHeight * kCenterFactor;
    button->setPosition(
        cocos2d::Vec2(kDetailSidePadding + kUpgradeButtonWidth * kCenterFactor, buttonCenterY));
    button->addClickEventListener([this, kind](cocos2d::Ref* sender) {
        static_cast<void>(sender);
        if (_onUpgradeRequested)
        {
            _onUpgradeRequested(kind);
        }
    });
    _detailNode->addChild(button);

    buttonTitle->setPosition(
        cocos2d::Vec2(kUpgradeButtonWidth * kCenterFactor, kUpgradeButtonHeight * kCenterFactor));
    button->addChild(buttonTitle);

    struct DetailLine
    {
        std::string text;
        float leftX;
        float centerY;
        float fontSize;
        cocos2d::Color3B color;
    };

    const float costRowCenterY = cursorY - kUpgradeButtonHeight - kUpgradeRowGap
        - kUpgradeCostRowHeight * kCenterFactor;
    const DetailLine lines[] = {
        {texts.level, kUpgradeLevelLeftX, buttonCenterY, kUpgradeTextFontSize, kStatTextColor},
        {texts.delta, kUpgradeDeltaLeftX, buttonCenterY, kUpgradeTextFontSize, kStatTextColor},
        {kCostCaption, kCostCaptionLeftX, costRowCenterY, kUpgradeTextFontSize, kStatTextColor},
        {texts.cost, kCostAmountLeftX, costRowCenterY, kUpgradeTextFontSize, pickCostColor(texts.affordable)},
    };

    for (const DetailLine& line : lines)
    {
        cocos2d::Label* label = PixelWidgets::createLabel(line.text, line.fontSize, line.color);
        if (label == nullptr)
        {
            return false;
        }

        label->setAnchorPoint(cocos2d::Vec2(0.0F, kCenterFactor));
        label->setPosition(cocos2d::Vec2(line.leftX, line.centerY));
        _detailNode->addChild(label);
    }

    cocos2d::Sprite* goldIcon =
        PixelWidgets::createSprite(std::string(PixelWidgets::kUiDirectory) + kGoldSmallIconImageFile);
    if (goldIcon == nullptr)
    {
        return false;
    }

    goldIcon->setPosition(cocos2d::Vec2(kCostIconCenterX, costRowCenterY));
    _detailNode->addChild(goldIcon);

    cursorY -= kUpgradeButtonHeight + kUpgradeRowGap + kUpgradeCostRowHeight;
    return true;
}

bool HeroCardView::_setUpDescriptions(const BattleHeroSnapshot& hero, float& cursorY)
{
    cursorY -= kDescriptionTopGap;

    cocos2d::Label* description = PixelWidgets::createWrappedLabel(hero.description,
                                                                  kDescriptionFontSize,
                                                                  kDescriptionTextColor,
                                                                  kDetailContentWidth,
                                                                  cocos2d::TextHAlignment::CENTER);
    if (description == nullptr)
    {
        return false;
    }

    description->setAnchorPoint(cocos2d::Vec2(0.0F, 1.0F));
    description->setPosition(cocos2d::Vec2(kDetailSidePadding, cursorY));
    _detailNode->addChild(description);
    cursorY -= description->getContentSize().height;

    for (const BattleHeroSkillSnapshot& skill : hero.skills)
    {
        cursorY -= kSkillEntryGap;

        const cocos2d::Color3B titleColor = skill.unlocked ? kSkillTextColor : kLockedTextColor;
        cocos2d::Label* title =
            PixelWidgets::createLabel(buildSkillTitle(skill), kSkillTitleFontSize, titleColor);
        if (title == nullptr)
        {
            return false;
        }

        title->setAnchorPoint(cocos2d::Vec2(0.0F, 1.0F));
        title->setPosition(cocos2d::Vec2(kDetailSidePadding, cursorY));
        _detailNode->addChild(title);
        cursorY -= title->getContentSize().height + kSkillTitleGap;

        cocos2d::Label* skillDescription =
            PixelWidgets::createWrappedLabel(buildSkillDescription(hero, skill),
                                             kSkillDescriptionFontSize,
                                             skill.unlocked ? kDescriptionTextColor : kLockedTextColor,
                                             kDetailContentWidth,
                                             cocos2d::TextHAlignment::LEFT);
        if (skillDescription == nullptr)
        {
            return false;
        }

        skillDescription->setAnchorPoint(cocos2d::Vec2(0.0F, 1.0F));
        skillDescription->setPosition(cocos2d::Vec2(kDetailSidePadding, cursorY));
        _detailNode->addChild(skillDescription);
        cursorY -= skillDescription->getContentSize().height;
    }

    return true;
}

bool HeroCardView::_setUpToggleInput()
{
    cocos2d::EventListenerTouchOneByOne* listener = cocos2d::EventListenerTouchOneByOne::create();
    if (listener == nullptr)
    {
        cocos2d::log("[HeroCardView] failed to create toggle listener");
        return false;
    }

    // 不吞掉触摸：英雄栏要靠同一个触摸序列滚动，吞掉会让列表滑不动。
    listener->setSwallowTouches(false);
    listener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event* event) {
        static_cast<void>(event);
        return touch != nullptr && _isInsideFoldedArea(touch->getLocation());
    };
    listener->onTouchEnded = [this](cocos2d::Touch* touch, cocos2d::Event* event) {
        static_cast<void>(event);
        if (touch == nullptr || !_isInsideFoldedArea(touch->getLocation()))
        {
            return;
        }

        // 位移超过阈值说明玩家在滑动列表，不当作点击。
        const cocos2d::Vec2 movement = touch->getLocation() - touch->getStartLocation();
        if (movement.length() > kTapMovementTolerance)
        {
            return;
        }

        if (_onToggleRequested)
        {
            _onToggleRequested();
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

bool HeroCardView::_isInsideFoldedArea(const cocos2d::Vec2& worldPoint) const
{
    // 命中区域只取折叠区：展开区里的按钮由自己处理点击，不应再触发收起。
    const cocos2d::Vec2 localPoint = convertToNodeSpace(worldPoint);
    const cocos2d::Rect foldedArea(0.0F, -kCardHeight, kCardWidth, kCardHeight);
    return foldedArea.containsPoint(localPoint);
}

}  // namespace DemonRealm
