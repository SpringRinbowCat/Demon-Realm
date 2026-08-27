#include "Presentation/BattleView.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <string>
#include <vector>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Presentation/Format/NumberFormatter.hpp"

namespace DemonRealm
{
namespace
{

/// 资源目录前缀，均相对 Resources 根目录。
/// 目录拼接暂时留在表现层；接入 Infrastructure 资源模块后再收口到那里。
const char* const kBackgroundDirectory = "Textures/Pixel/Backgrounds/";
const char* const kBossDirectory = "Textures/Pixel/Bosses/";
const char* const kHeroDirectory = "Textures/Pixel/Heroes/";
const char* const kUiDirectory = "Textures/Pixel/UI/";

/// 金币图标文件名。
const char* const kGoldIconImageFile = "图标_金币.png";

/// 底部栏按钮贴图文件名。
const char* const kBottomBarNormalImageFile = "按钮_底部栏_常态.png";
const char* const kBottomBarPressedImageFile = "按钮_底部栏_按下.png";

/// 英雄卡片面板贴图文件名。
const char* const kHeroCardPanelImageFile = "面板_英雄卡片.png";

/// 文字使用的系统字体名；项目暂未接入像素字体资源，中文依赖系统字体回退。
const char* const kFontName = "Arial";

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

/// 单个英雄卡片布局，卡片尺寸与 面板_英雄卡片.png 一致。
const float kHeroCardWidth = 520.0F;
const float kHeroCardHeight = 110.0F;
const float kHeroCardGap = 10.0F;
const float kHeroPortraitCenterX = 60.0F;

/// 卡片内文字的固定列位置。
/// 英雄名和等级各自占一列，不靠空格拉开距离，保证多条英雄栏之间名字对齐、等级也对齐。
const float kHeroNameLeftX = 120.0F;
const float kHeroLevelLeftX = 300.0F;

/// 卡片内文字的行位置；共四行：名称与等级、攻击力、攻击间隔、已解锁技能。
const float kHeroFirstLineY = 88.0F;
const float kHeroLineSpacing = 23.0F;
const float kHeroNameFontSize = 20.0F;
const float kHeroStatFontSize = 16.0F;

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
const cocos2d::Color3B kHeroNameTextColor(255, 220, 61);
const cocos2d::Color3B kHeroStatTextColor(240, 230, 225);
const cocos2d::Color3B kHeroSkillTextColor(255, 173, 28);
const cocos2d::Color3B kBottomBarActiveTextColor(255, 220, 61);
const cocos2d::Color3B kBottomBarInactiveTextColor(200, 190, 185);

/// 底部栏入口项及其文字。
struct BottomBarEntry
{
    BattleBottomBarItem item;
    const char* label;
};

/// 英雄卡片内的一条文字，位置按列和行分别固定。
struct HeroCardTextLine
{
    /// 文字内容。
    std::string text;

    /// 左对齐的列位置，卡片内局部坐标。
    float leftX;

    /// 行位置，卡片内局部坐标。
    float lineY;

    /// 字号。
    float fontSize;

    /// 文字颜色。
    cocos2d::Color3B color;
};

/// 底部栏入口顺序；当前只有战斗页面已实现，其余为占位入口。
const BottomBarEntry kBottomBarEntries[] = {
    {BattleBottomBarItem::Battle, "战斗"},
    {BattleBottomBarItem::Heroes, "英雄"},
    {BattleBottomBarItem::Shop, "商店"},
    {BattleBottomBarItem::Treasures, "宝物"},
    {BattleBottomBarItem::Settings, "设置"},
};

/// 像素贴图统一使用最近邻采样，避免线性过滤模糊像素格。
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

/// 创建像素贴图精灵并设置最近邻采样。
/// 返回值：加载失败时返回 nullptr。
cocos2d::Sprite* createPixelSprite(const std::string& path)
{
    cocos2d::Sprite* sprite = cocos2d::Sprite::create(path);
    if (sprite == nullptr)
    {
        cocos2d::log("[BattleView] failed to load image: %s", path.c_str());
        return nullptr;
    }

    applyPixelTextureFilter(sprite);
    return sprite;
}

/// 创建系统字体文字节点。
/// 返回值：创建失败时返回 nullptr。
cocos2d::Label* createLabel(const std::string& text, float fontSize, const cocos2d::Color3B& color)
{
    cocos2d::Label* label = cocos2d::Label::createWithSystemFont(text, kFontName, fontSize);
    if (label == nullptr)
    {
        cocos2d::log("[BattleView] failed to create label: %s", text.c_str());
        return nullptr;
    }

    label->setTextColor(cocos2d::Color4B(color));
    return label;
}

/// 把攻击间隔格式化为展示文字，小数位最少 2 位、最多 4 位。
/// 参数 canonicalSeconds：规范化定点小数字符串。
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
    if (heroes.size() != _heroCardLabels.size())
    {
        cocos2d::log("[BattleView] hero count changed; rebuilding the hero list is not implemented yet");
        return;
    }

    for (std::size_t index = 0; index < heroes.size(); ++index)
    {
        const BattleHeroSnapshot& hero = heroes[index];
        const BattleHeroSnapshot& previous = _snapshot.heroes[index];
        const HeroCardLabels& labels = _heroCardLabels[index];

        if (labels.level != nullptr && hero.level != previous.level)
        {
            labels.level->setString(cocos2d::StringUtils::format("等级：%d", hero.level));
        }

        if (labels.attack != nullptr && hero.attack != previous.attack)
        {
            labels.attack->setString("攻击力：" + NumberFormatter::formatIntegerWithGroups(hero.attack));
        }

        if (labels.attackInterval != nullptr && hero.attackIntervalSeconds != previous.attackIntervalSeconds)
        {
            labels.attackInterval->setString("攻击间隔：" + formatIntervalSeconds(hero.attackIntervalSeconds));
        }

        if (labels.skills != nullptr && hero.unlockedSkillNames != previous.unlockedSkillNames)
        {
            labels.skills->setString("技能：" + joinUnlockedSkillNames(hero.unlockedSkillNames));
        }
    }

    _snapshot.heroes = heroes;
}

void BattleView::setOnBottomBarItemSelected(const BottomBarSelectionCallback& callback)
{
    _onBottomBarItemSelected = callback;
}

void BattleView::setOnBossTapped(const BossTapCallback& callback)
{
    _onBossTapped = callback;
}

bool BattleView::_setUpBackground()
{
    cocos2d::Sprite* background =
        createPixelSprite(std::string(kBackgroundDirectory) + _snapshot.backgroundImageFile);
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
    cocos2d::Sprite* goldIcon = createPixelSprite(std::string(kUiDirectory) + kGoldIconImageFile);
    if (goldIcon == nullptr)
    {
        return false;
    }

    goldIcon->setPosition(layoutPosition(kGoldIconCenterX, kGoldBarCenterY));
    addChild(goldIcon, kGoldBarZOrder);

    cocos2d::Label* goldAmount =
        createLabel(NumberFormatter::formatIntegerWithGroups(_snapshot.status.goldAmount),
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
    cocos2d::Sprite* boss = createPixelSprite(std::string(kBossDirectory) + _snapshot.bossImageFile);
    if (boss == nullptr)
    {
        return false;
    }

    boss->setPosition(layoutPosition(kDesignWidth * kCenterFactor, kBossCenterY));
    addChild(boss, kBossZOrder);
    _bossSprite = boss;

    cocos2d::Label* remainingHp =
        createLabel(NumberFormatter::formatIntegerWithGroups(_snapshot.status.bossRemainingHp),
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

cocos2d::Node* BattleView::_createHeroCard(const BattleHeroSnapshot& hero, HeroCardLabels& labels)
{
    cocos2d::Node* card = cocos2d::Node::create();
    if (card == nullptr)
    {
        cocos2d::log("[BattleView] failed to create hero card node");
        return nullptr;
    }

    card->setContentSize(cocos2d::Size(kHeroCardWidth, kHeroCardHeight));

    cocos2d::Sprite* panel = createPixelSprite(std::string(kUiDirectory) + kHeroCardPanelImageFile);
    cocos2d::Sprite* portrait = createPixelSprite(std::string(kHeroDirectory) + hero.cardImageFile);
    if (panel == nullptr || portrait == nullptr)
    {
        return nullptr;
    }

    panel->setPosition(cocos2d::Vec2(kHeroCardWidth * kCenterFactor, kHeroCardHeight * kCenterFactor));
    card->addChild(panel);
    portrait->setPosition(cocos2d::Vec2(kHeroPortraitCenterX, kHeroCardHeight * kCenterFactor));
    card->addChild(portrait);

    // 第一行的名称与等级各自占固定列；攻击力与攻击间隔各占一行。
    const float nameRowY = kHeroFirstLineY;
    const float attackRowY = kHeroFirstLineY - kHeroLineSpacing;
    const float intervalRowY = kHeroFirstLineY - kHeroLineSpacing * 2.0F;
    const float skillRowY = kHeroFirstLineY - kHeroLineSpacing * 3.0F;

    const std::vector<HeroCardTextLine> lines = {
        {hero.displayName, kHeroNameLeftX, nameRowY, kHeroNameFontSize, kHeroNameTextColor},
        {cocos2d::StringUtils::format("等级：%d", hero.level),
         kHeroLevelLeftX,
         nameRowY,
         kHeroNameFontSize,
         kHeroNameTextColor},
        {"攻击力：" + NumberFormatter::formatIntegerWithGroups(hero.attack),
         kHeroNameLeftX,
         attackRowY,
         kHeroStatFontSize,
         kHeroStatTextColor},
        {"攻击间隔：" + formatIntervalSeconds(hero.attackIntervalSeconds),
         kHeroNameLeftX,
         intervalRowY,
         kHeroStatFontSize,
         kHeroStatTextColor},
        {"技能：" + joinUnlockedSkillNames(hero.unlockedSkillNames),
         kHeroNameLeftX,
         skillRowY,
         kHeroStatFontSize,
         kHeroSkillTextColor},
    };

    // 与上面的行顺序一致：名称、等级、攻击力、攻击间隔、技能；名称不随战斗变化，无需保存。
    cocos2d::Label** const refreshableLabels[] = {
        nullptr,
        &labels.level,
        &labels.attack,
        &labels.attackInterval,
        &labels.skills,
    };

    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const HeroCardTextLine& line = lines[index];
        cocos2d::Label* label = createLabel(line.text, line.fontSize, line.color);
        if (label == nullptr)
        {
            return nullptr;
        }

        label->setAnchorPoint(cocos2d::Vec2(0.0F, kCenterFactor));
        label->setPosition(cocos2d::Vec2(line.leftX, line.lineY));
        card->addChild(label);

        if (index < sizeof(refreshableLabels) / sizeof(refreshableLabels[0])
            && refreshableLabels[index] != nullptr)
        {
            *refreshableLabels[index] = label;
        }
    }

    return card;
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

    // 内容高度不足一屏时按可见高度处理，避免卡片被顶到可见区域之外。
    const float cardCount = static_cast<float>(_snapshot.heroes.size());
    const float requiredHeight = cardCount * (kHeroCardHeight + kHeroCardGap) + kHeroCardGap;
    const float innerHeight = std::max(requiredHeight, kHeroListHeight);
    heroList->setInnerContainerSize(cocos2d::Size(kHeroListWidth, innerHeight));

    _heroCardLabels.assign(_snapshot.heroes.size(), HeroCardLabels());
    for (std::size_t index = 0; index < _snapshot.heroes.size(); ++index)
    {
        cocos2d::Node* card = _createHeroCard(_snapshot.heroes[index], _heroCardLabels[index]);
        if (card == nullptr)
        {
            return false;
        }

        // 卡片从上往下排列，第一个英雄在列表顶部。
        const float cardBottomY = innerHeight - kHeroCardGap - kHeroCardHeight
            - static_cast<float>(index) * (kHeroCardHeight + kHeroCardGap);
        card->setPosition(cocos2d::Vec2((kHeroListWidth - kHeroCardWidth) * kCenterFactor, cardBottomY));
        heroList->addChild(card);
    }

    addChild(heroList, kHeroCardZOrder);
    return true;
}

bool BattleView::_setUpBottomBar()
{
    cocos2d::Vector<cocos2d::MenuItem*> items;
    const std::size_t entryCount = sizeof(kBottomBarEntries) / sizeof(kBottomBarEntries[0]);
    for (std::size_t index = 0; index < entryCount; ++index)
    {
        const BottomBarEntry& entry = kBottomBarEntries[index];
        cocos2d::Sprite* normalSprite = createPixelSprite(std::string(kUiDirectory) + kBottomBarNormalImageFile);
        cocos2d::Sprite* pressedSprite =
            createPixelSprite(std::string(kUiDirectory) + kBottomBarPressedImageFile);
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
        cocos2d::Label* label = createLabel(
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
