#include "Presentation/BattleView.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <string>
#include <vector>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Presentation/Format/NumberFormatter.hpp"
#include "Presentation/PixelWidgets.hpp"

namespace DemonRealm
{
namespace
{

/// 金币图标文件名。
const char* const kGoldIconImageFile = "图标_金币.png";

/// 底部栏按钮贴图文件名。
const char* const kBottomBarNormalImageFile = "按钮_底部栏_常态.png";
const char* const kBottomBarPressedImageFile = "按钮_底部栏_按下.png";

/// 设计分辨率，与像素资源基准一致。
const float kDesignWidth = 540.0F;
const float kDesignHeight = 960.0F;

/// 取中点使用的比例系数。
const float kCenterFactor = 0.5F;

/// 顶部金币栏布局。
const float kGoldIconCenterX = 45.0F;
const float kGoldBarCenterY = 900.0F;
const float kGoldTextLeftX = 85.0F;
const float kGoldFontSize = 30.0F;

/// Boss 区域布局；Boss 区域只占背景上半屏（y 从 480 到 960）。
const float kBossCenterY = 700.0F;
const float kBossHpCenterY = 520.0F;
const float kBossHpFontSize = 40.0F;

/// 下半屏英雄栏布局；可滑动区域位于底部栏之上、Boss 区域之下。
const float kHeroListBottomY = 120.0F;
const float kHeroListHeight = 360.0F;
const float kHeroListWidth = 540.0F;

/// 英雄卡片宽度与间隔；卡片内部布局由 HeroCardView 负责。
const float kHeroCardWidth = 520.0F;
const float kHeroCardGap = 10.0F;

/// 底部栏布局。
const float kBottomBarButtonSize = 100.0F;
const float kBottomBarGap = 10.0F;
const float kBottomBarFirstCenterX = 50.0F;
const float kBottomBarCenterY = 60.0F;
const float kBottomBarFontSize = 22.0F;

/// 渲染层级。
const int kBackgroundZOrder = 0;
const int kBossZOrder = 10;
const int kHeroCardZOrder = 20;
const int kBottomBarZOrder = 30;
const int kGoldBarZOrder = 40;

/// 文字颜色，取自项目既有调色板。
const cocos2d::Color3B kGoldTextColor(255, 220, 61);
const cocos2d::Color3B kBossHpTextColor(255, 173, 28);
const cocos2d::Color3B kBottomBarActiveTextColor(255, 220, 61);
const cocos2d::Color3B kBottomBarInactiveTextColor(200, 190, 185);

/// 底部栏入口项及其文字。
struct BottomBarEntry
{
    BattleBottomBarItem item;
    const char* label;
};

/// 底部栏入口顺序；当前只有战斗页面已实现，其余为占位入口。
const BottomBarEntry kBottomBarEntries[] = {
    {BattleBottomBarItem::Battle, "战斗"},
    {BattleBottomBarItem::Heroes, "英雄"},
    {BattleBottomBarItem::Shop, "商店"},
    {BattleBottomBarItem::Treasures, "宝物"},
    {BattleBottomBarItem::Settings, "设置"},
};

/// 按设计坐标计算节点位置。
/// 参数 designX/designY：以可见区域左下角为原点的设计坐标。
cocos2d::Vec2 layoutPosition(float designX, float designY)
{
    const cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director == nullptr)
    {
        return cocos2d::Vec2(designX, designY);
    }

    const cocos2d::Vec2 visibleOrigin = director->getVisibleOrigin();
    return cocos2d::Vec2(visibleOrigin.x + designX, visibleOrigin.y + designY);
}

}  // namespace

BattleView* BattleView::create(const BattleSnapshot& snapshot)
{
    BattleView* view = new (std::nothrow) BattleView();
    if (view == nullptr)
    {
        cocos2d::log("[BattleView] failed to allocate view");
        return nullptr;
    }

    if (!view->initWithSnapshot(snapshot))
    {
        // 初始化失败的对象还没有进入自动释放池，这里直接释放，避免泄漏。
        delete view;
        return nullptr;
    }

    view->autorelease();
    return view;
}

bool BattleView::initWithSnapshot(const BattleSnapshot& snapshot)
{
    if (!cocos2d::Node::init())
    {
        return false;
    }

    _snapshot = snapshot;

    return _setUpBackground() && _setUpGoldBar() && _setUpBossArea() && _setUpBossTapInput()
        && _setUpHeroList() && _setUpBottomBar();
}

bool BattleView::_setUpBossTapInput()
{
    if (_bossSprite == nullptr)
    {
        cocos2d::log("[BattleView] boss sprite is required before wiring tap input");
        return false;
    }

    cocos2d::EventListenerTouchOneByOne* listener = cocos2d::EventListenerTouchOneByOne::create();
    if (listener == nullptr)
    {
        cocos2d::log("[BattleView] failed to create boss tap listener");
        return false;
    }

    // 只吞掉落在 Boss 贴图上的触摸，其他位置的触摸继续向下传递给英雄栏和底部栏。
    listener->setSwallowTouches(false);
    listener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event* event) {
        static_cast<void>(event);
        if (_bossSprite == nullptr || touch == nullptr)
        {
            return false;
        }

        // 命中区域取 Boss 贴图自身的矩形；贴图与本节点在同一坐标系下比较。
        const cocos2d::Vec2 localPoint = convertToNodeSpace(touch->getLocation());
        if (!_bossSprite->getBoundingBox().containsPoint(localPoint))
        {
            return false;
        }

        if (_onBossTapped)
        {
            _onBossTapped();
        }

        return true;
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

void BattleView::updateStatus(const BattleStatusSnapshot& status)
{
    // 只有原始数值字符串变化时才重设文字：Label 每次 setString 都要重新排版和上传纹理，
    // 数值没变时跳过可以让空转的帧几乎没有额外开销。
    if (_goldAmountLabel != nullptr && status.goldAmount != _snapshot.status.goldAmount)
    {
        _goldAmountLabel->setString(NumberFormatter::formatIntegerWithGroups(status.goldAmount));
    }

    if (_bossRemainingHpLabel != nullptr && status.bossRemainingHp != _snapshot.status.bossRemainingHp)
    {
        _bossRemainingHpLabel->setString(NumberFormatter::formatIntegerWithGroups(status.bossRemainingHp));
    }

    _snapshot.status = status;
}

void BattleView::updateHeroes(const std::vector<BattleHeroSnapshot>& heroes)
{
    // 快照数量与卡片数量不一致说明英雄列表发生了增删，这需要重建卡片而不是改文字；
    // 召唤新英雄的流程尚未实现，这里只记录并跳过，避免把数值写到错位的卡片上。
    if (heroes.size() != _heroCards.size())
    {
        cocos2d::log("[BattleView] hero count changed; rebuilding the hero list is not implemented yet");
        return;
    }

    for (std::size_t index = 0; index < heroes.size(); ++index)
    {
        _heroCards[index]->updateHero(heroes[index]);
    }

    _snapshot.heroes = heroes;

    // 升级会重建详情区，展开中的卡片高度可能因此变化，重排一次保证卡片不重叠。
    _relayoutHeroList();
}

void BattleView::setOnBottomBarItemSelected(const BottomBarSelectionCallback& callback)
{
    _onBottomBarItemSelected = callback;
}

void BattleView::setOnBossTapped(const BossTapCallback& callback)
{
    _onBossTapped = callback;
}

void BattleView::setOnHeroUpgradeRequested(const HeroUpgradeCallback& callback)
{
    _onHeroUpgradeRequested = callback;
}

bool BattleView::_setUpBackground()
{
    cocos2d::Sprite* background =
        PixelWidgets::createSprite(std::string(PixelWidgets::kBackgroundDirectory) + _snapshot.backgroundImageFile);
    if (background == nullptr)
    {
        return false;
    }

    // 背景按设计分辨率 1:1 呈现，不做运行时缩放，保持既有像素格密度。
    background->setPosition(layoutPosition(kDesignWidth * kCenterFactor, kDesignHeight * kCenterFactor));
    addChild(background, kBackgroundZOrder);
    return true;
}

bool BattleView::_setUpGoldBar()
{
    cocos2d::Sprite* goldIcon = PixelWidgets::createSprite(std::string(PixelWidgets::kUiDirectory) + kGoldIconImageFile);
    if (goldIcon == nullptr)
    {
        return false;
    }

    goldIcon->setPosition(layoutPosition(kGoldIconCenterX, kGoldBarCenterY));
    addChild(goldIcon, kGoldBarZOrder);

    cocos2d::Label* goldAmount =
        PixelWidgets::createLabel(NumberFormatter::formatIntegerWithGroups(_snapshot.status.goldAmount),
                    kGoldFontSize,
                    kGoldTextColor);
    if (goldAmount == nullptr)
    {
        return false;
    }

    goldAmount->setAnchorPoint(cocos2d::Vec2(0.0F, kCenterFactor));
    goldAmount->setPosition(layoutPosition(kGoldTextLeftX, kGoldBarCenterY));
    addChild(goldAmount, kGoldBarZOrder);
    _goldAmountLabel = goldAmount;
    return true;
}

bool BattleView::_setUpBossArea()
{
    cocos2d::Sprite* boss = PixelWidgets::createSprite(std::string(PixelWidgets::kBossDirectory) + _snapshot.bossImageFile);
    if (boss == nullptr)
    {
        return false;
    }

    boss->setPosition(layoutPosition(kDesignWidth * kCenterFactor, kBossCenterY));
    addChild(boss, kBossZOrder);
    _bossSprite = boss;

    cocos2d::Label* remainingHp =
        PixelWidgets::createLabel(NumberFormatter::formatIntegerWithGroups(_snapshot.status.bossRemainingHp),
                    kBossHpFontSize,
                    kBossHpTextColor);
    if (remainingHp == nullptr)
    {
        return false;
    }

    remainingHp->setPosition(layoutPosition(kDesignWidth * kCenterFactor, kBossHpCenterY));
    addChild(remainingHp, kBossZOrder);
    _bossRemainingHpLabel = remainingHp;
    return true;
}

bool BattleView::_setUpHeroList()
{
    cocos2d::ui::ScrollView* heroList = cocos2d::ui::ScrollView::create();
    if (heroList == nullptr)
    {
        cocos2d::log("[BattleView] failed to create hero list scroll view");
        return false;
    }

    heroList->setDirection(cocos2d::ui::ScrollView::Direction::VERTICAL);
    heroList->setContentSize(cocos2d::Size(kHeroListWidth, kHeroListHeight));
    heroList->setPosition(layoutPosition(0.0F, kHeroListBottomY));
    heroList->setBounceEnabled(true);
    addChild(heroList, kHeroCardZOrder);
    _heroList = heroList;

    _heroCards.clear();
    _heroCards.reserve(_snapshot.heroes.size());
    for (std::size_t index = 0; index < _snapshot.heroes.size(); ++index)
    {
        HeroCardView* card = HeroCardView::create(_snapshot.heroes[index]);
        if (card == nullptr)
        {
            return false;
        }

        card->setOnToggleRequested([this, index]() { _onHeroCardToggled(index); });
        card->setOnUpgradeRequested([this, index](HeroUpgradeKind kind) {
            if (_onHeroUpgradeRequested)
            {
                _onHeroUpgradeRequested(index, kind);
            }
        });

        heroList->addChild(card);
        _heroCards.push_back(card);
    }

    _relayoutHeroList();
    return true;
}

void BattleView::_relayoutHeroList()
{
    if (_heroList == nullptr)
    {
        return;
    }

    float requiredHeight = kHeroCardGap;
    for (const HeroCardView* card : _heroCards)
    {
        requiredHeight += card->getPreferredHeight() + kHeroCardGap;
    }

    // 内容高度不足一屏时按可见高度处理，避免卡片被顶到可见区域之外。
    const float innerHeight = std::max(requiredHeight, kHeroListHeight);
    _heroList->setInnerContainerSize(cocos2d::Size(kHeroListWidth, innerHeight));

    // 卡片从上往下排列，第一个英雄在列表顶部；卡片内容从自身原点向下生长，
    // 因此这里定位的是每张卡片的顶边。
    const float cardLeftX = (kHeroListWidth - kHeroCardWidth) * kCenterFactor;
    float cardTopY = innerHeight - kHeroCardGap;
    for (HeroCardView* card : _heroCards)
    {
        card->setPosition(cocos2d::Vec2(cardLeftX, cardTopY));
        cardTopY -= card->getPreferredHeight() + kHeroCardGap;
    }
}

void BattleView::_onHeroCardToggled(std::size_t heroIndex)
{
    if (heroIndex >= _heroCards.size())
    {
        return;
    }

    const bool shouldExpand = !_heroCards[heroIndex]->isExpanded();
    for (std::size_t index = 0; index < _heroCards.size(); ++index)
    {
        // 同一时间只展开一张卡片，避免列表被多份详情撑得过长。
        _heroCards[index]->setExpanded(index == heroIndex && shouldExpand);
    }

    _relayoutHeroList();
}

bool BattleView::_setUpBottomBar()
{
    cocos2d::Vector<cocos2d::MenuItem*> items;
    const std::size_t entryCount = sizeof(kBottomBarEntries) / sizeof(kBottomBarEntries[0]);
    for (std::size_t index = 0; index < entryCount; ++index)
    {
        const BottomBarEntry& entry = kBottomBarEntries[index];
        cocos2d::Sprite* normalSprite = PixelWidgets::createSprite(std::string(PixelWidgets::kUiDirectory) + kBottomBarNormalImageFile);
        cocos2d::Sprite* pressedSprite =
            PixelWidgets::createSprite(std::string(PixelWidgets::kUiDirectory) + kBottomBarPressedImageFile);
        if (normalSprite == nullptr || pressedSprite == nullptr)
        {
            return false;
        }

        const BattleBottomBarItem item = entry.item;
        cocos2d::MenuItemSprite* button = cocos2d::MenuItemSprite::create(
            normalSprite,
            pressedSprite,
            [this, item](cocos2d::Ref* sender) {
                static_cast<void>(sender);
                _onBottomBarItemClicked(item);
            });
        if (button == nullptr)
        {
            cocos2d::log("[BattleView] failed to create bottom bar button");
            return false;
        }

        const bool isCurrentPage = entry.item == BattleBottomBarItem::Battle;
        cocos2d::Label* label = PixelWidgets::createLabel(
            entry.label,
            kBottomBarFontSize,
            isCurrentPage ? kBottomBarActiveTextColor : kBottomBarInactiveTextColor);
        if (label == nullptr)
        {
            return false;
        }

        label->setPosition(cocos2d::Vec2(kBottomBarButtonSize * kCenterFactor,
                                         kBottomBarButtonSize * kCenterFactor));
        button->addChild(label);
        button->setPosition(layoutPosition(
            kBottomBarFirstCenterX + static_cast<float>(index) * (kBottomBarButtonSize + kBottomBarGap),
            kBottomBarCenterY));
        items.pushBack(button);
    }

    cocos2d::Menu* menu = cocos2d::Menu::createWithArray(items);
    if (menu == nullptr)
    {
        cocos2d::log("[BattleView] failed to create bottom bar menu");
        return false;
    }

    menu->setPosition(cocos2d::Vec2::ZERO);
    addChild(menu, kBottomBarZOrder);
    return true;
}

void BattleView::_onBottomBarItemClicked(BattleBottomBarItem item)
{
    // 视图只回传“哪个入口被点击”这一事实，页面切换由上层决定。
    cocos2d::log("[BattleView] bottom bar item clicked: %d", static_cast<int>(item));

    if (_onBottomBarItemSelected)
    {
        _onBottomBarItemSelected(item);
    }
}

}  // namespace DemonRealm
