#include "Infrastructure/Config/ConfigService.hpp"

#include <string>

#include "cocos2d.h"
#include "json/document.h"

#include "Domain/Numeric/Decimal.hpp"

namespace DemonRealm
{
namespace
{

/// Boss 配置文件路径，相对 Resources 根目录。
const char* const kBossConfigPath = "Config/bosses.json";

/// 英雄配置文件路径，相对 Resources 根目录。
const char* const kHeroConfigPath = "Config/heroes.json";

/// 当前支持的配置结构版本。
const int kSupportedSchemaVersion = 1;

/// 技能可解锁的最小英雄等级。
const int kMinimumUnlockLevel = 1;

/// 英雄等级的最小取值，也是攻击力等级区间的起点。
const int kMinimumHeroLevel = 1;

/// 支持的技能触发时机标识。
const char* const kTapAttackTrigger = "tapAttack";

/// 支持的技能效果类型标识。
const char* const kDamageEffectType = "damage";
const char* const kPermanentAttackGrowthEffectType = "permanentAttackGrowth";

/// 读取并解析配置文件。
/// 参数 path：相对 Resources 根目录的路径。
/// 参数 document：解析结果输出。
/// 返回值：文件存在、内容非空、JSON 合法且顶层为对象时返回 true。
bool parseConfigFile(const char* path, rapidjson::Document& document)
{
    const std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(path);
    if (content.empty())
    {
        cocos2d::log("[ConfigService] config file is missing or empty: %s", path);
        return false;
    }

    document.Parse(content.c_str());
    if (document.HasParseError() || !document.IsObject())
    {
        cocos2d::log("[ConfigService] failed to parse config file: %s", path);
        return false;
    }

    if (!document.HasMember("schemaVersion") || !document["schemaVersion"].IsInt()
        || document["schemaVersion"].GetInt() != kSupportedSchemaVersion)
    {
        cocos2d::log("[ConfigService] unsupported schemaVersion in %s, expected %d",
                     path,
                     kSupportedSchemaVersion);
        return false;
    }

    return true;
}

/// 读取必填字符串字段。
/// 返回值：字段存在且为非空字符串时返回 true，并写入 value。
bool readRequiredString(const rapidjson::Value& owner, const char* key, std::string& value)
{
    if (!owner.HasMember(key) || !owner[key].IsString() || owner[key].GetStringLength() == 0)
    {
        cocos2d::log("[ConfigService] missing or invalid string field: %s", key);
        return false;
    }

    value = owner[key].GetString();
    return true;
}

/// 读取必填正数字段。
///
/// 数值在配置里写成字符串，因为血量、攻击力这类数值会膨胀到 double 无法精确表示的量级，
/// 交给 rapidjson 转成数字会在解析阶段就丢精度。这里只校验格式与正负，原样保留字符串。
///
/// 参数 owner：所属对象。
/// 参数 key：字段名。
/// 参数 value：字段原始字符串输出。
/// 返回值：字段存在、是合法的非负十进制字符串且大于 0 时返回 true。
bool readRequiredPositiveNumber(const rapidjson::Value& owner, const char* key, std::string& value)
{
    std::string text;
    if (!readRequiredString(owner, key, text))
    {
        cocos2d::log("[ConfigService] numeric field must be a decimal string: %s", key);
        return false;
    }

    Decimal parsed;
    if (!Decimal::tryParse(text, parsed))
    {
        cocos2d::log("[ConfigService] field is not a valid decimal string: %s = %s", key, text.c_str());
        return false;
    }

    if (parsed.isZero())
    {
        cocos2d::log("[ConfigService] field must be greater than zero: %s", key);
        return false;
    }

    value = text;
    return true;
}

/// 读取必填概率字段。
///
/// 参数 owner：所属对象。
/// 参数 key：字段名。
/// 参数 value：字段原始字符串输出。
/// 返回值：字段是合法的十进制字符串且取值在 0 到 1 之间（含 1、不含 0）时返回 true。
bool readRequiredProbability(const rapidjson::Value& owner, const char* key, std::string& value)
{
    std::string text;
    if (!readRequiredPositiveNumber(owner, key, text))
    {
        return false;
    }

    Decimal probability;
    Decimal upperBound;
    if (!Decimal::tryParse(text, probability) || !Decimal::tryParse("1", upperBound))
    {
        return false;
    }

    if (probability.compare(upperBound) > 0)
    {
        cocos2d::log("[ConfigService] probability must not exceed 1: %s = %s", key, text.c_str());
        return false;
    }

    value = text;
    return true;
}

/// 读取 images 子对象中的必填文件名字段。
/// 返回值：images 存在且对应字段为非空字符串时返回 true。
bool readImageFileName(const rapidjson::Value& owner, const char* key, std::string& value)
{
    if (!owner.HasMember("images") || !owner["images"].IsObject())
    {
        cocos2d::log("[ConfigService] missing images object");
        return false;
    }

    return readRequiredString(owner["images"], key, value);
}

/// 解析单个 Boss 配置条目。
/// 返回值：字段齐全且合法时返回 true。
bool parseBoss(const rapidjson::Value& entry, BossConfig& boss)
{
    if (!entry.IsObject())
    {
        cocos2d::log("[ConfigService] boss entry is not an object");
        return false;
    }

    return readRequiredString(entry, "id", boss.id)
           && readRequiredString(entry, "displayName", boss.displayName)
           && readRequiredPositiveNumber(entry, "maxHp", boss.maxHp)
           && readImageFileName(entry, "background", boss.backgroundImageFile)
           && readImageFileName(entry, "idle", boss.idleImageFile);
}

/// 解析技能的 effect 子对象。
///
/// 只读取该效果类型需要的参数，未知效果类型直接判为配置错误，避免加载出一个没有行为的技能。
///
/// 参数 effect：effect 子对象。
/// 参数 skill：解析结果输出。
/// 返回值：效果类型已知且所需参数齐全合法时返回 true。
bool parseHeroSkillEffect(const rapidjson::Value& effect, HeroSkillConfig& skill)
{
    if (!readRequiredString(effect, "type", skill.effectType))
    {
        return false;
    }

    if (skill.effectType == kDamageEffectType)
    {
        return readRequiredPositiveNumber(effect, "attackMultiplier", skill.attackMultiplier);
    }

    if (skill.effectType == kPermanentAttackGrowthEffectType)
    {
        return readRequiredProbability(effect, "chance", skill.chance)
               && readRequiredPositiveNumber(effect, "levelProductDivisor", skill.levelProductDivisor);
    }

    cocos2d::log("[ConfigService] unsupported skill effect type: %s", skill.effectType.c_str());
    return false;
}

/// 解析单个技能配置条目。
/// 返回值：字段齐全且解锁等级、触发时机与效果参数合法时返回 true。
bool parseHeroSkill(const rapidjson::Value& entry, HeroSkillConfig& skill)
{
    if (!entry.IsObject())
    {
        cocos2d::log("[ConfigService] skill entry is not an object");
        return false;
    }

    if (!readRequiredString(entry, "id", skill.id)
        || !readRequiredString(entry, "displayName", skill.displayName)
        || !readRequiredString(entry, "description", skill.description))
    {
        return false;
    }

    if (!entry.HasMember("unlockLevel") || !entry["unlockLevel"].IsInt()
        || entry["unlockLevel"].GetInt() < kMinimumUnlockLevel)
    {
        cocos2d::log("[ConfigService] invalid unlockLevel for skill: %s", skill.id.c_str());
        return false;
    }

    skill.unlockLevel = entry["unlockLevel"].GetInt();

    if (!readRequiredString(entry, "trigger", skill.trigger))
    {
        return false;
    }

    if (skill.trigger != kTapAttackTrigger)
    {
        cocos2d::log("[ConfigService] unsupported skill trigger: %s", skill.trigger.c_str());
        return false;
    }

    if (!entry.HasMember("effect") || !entry["effect"].IsObject())
    {
        cocos2d::log("[ConfigService] missing effect object for skill: %s", skill.id.c_str());
        return false;
    }

    return parseHeroSkillEffect(entry["effect"], skill);
}

/// 解析攻击力等级的分段成长倍率。
///
/// 区间必须从 1 级开始、依次相接且不重叠，否则某些等级会取不到倍率，
/// 升级增量就会出现说不清的跳变，这类问题在运行期很难定位，因此在加载阶段直接拦住。
///
/// 参数 entry：英雄配置条目。
/// 参数 ranges：解析结果输出。
/// 返回值：区间齐全且连续时返回 true。
bool parseAttackLevelMultiplierRanges(const rapidjson::Value& entry,
                                      std::vector<HeroAttackLevelMultiplierRange>& ranges)
{
    if (!entry.HasMember("attackLevelMultiplierRanges") || !entry["attackLevelMultiplierRanges"].IsArray()
        || entry["attackLevelMultiplierRanges"].Empty())
    {
        cocos2d::log("[ConfigService] missing or empty attackLevelMultiplierRanges");
        return false;
    }

    int expectedMinLevel = kMinimumHeroLevel;
    for (const rapidjson::Value& rangeEntry : entry["attackLevelMultiplierRanges"].GetArray())
    {
        if (!rangeEntry.IsObject() || !rangeEntry.HasMember("minLevel") || !rangeEntry["minLevel"].IsInt()
            || !rangeEntry.HasMember("maxLevel") || !rangeEntry["maxLevel"].IsInt())
        {
            cocos2d::log("[ConfigService] invalid attack level multiplier range entry");
            return false;
        }

        HeroAttackLevelMultiplierRange range;
        range.minLevel = rangeEntry["minLevel"].GetInt();
        range.maxLevel = rangeEntry["maxLevel"].GetInt();
        if (range.minLevel != expectedMinLevel || range.maxLevel < range.minLevel)
        {
            cocos2d::log("[ConfigService] attack level ranges must start at %d and be continuous, got %d-%d",
                         expectedMinLevel,
                         range.minLevel,
                         range.maxLevel);
            return false;
        }

        if (!readRequiredPositiveNumber(rangeEntry, "multiplier", range.multiplier))
        {
            return false;
        }

        expectedMinLevel = range.maxLevel + 1;
        ranges.push_back(range);
    }

    return true;
}

/// 解析单个英雄配置条目，包含技能列表。
/// 返回值：字段齐全且合法时返回 true。
bool parseHero(const rapidjson::Value& entry, HeroConfig& hero)
{
    if (!entry.IsObject())
    {
        cocos2d::log("[ConfigService] hero entry is not an object");
        return false;
    }

    if (!readRequiredString(entry, "id", hero.id)
        || !readRequiredString(entry, "displayName", hero.displayName)
        || !readRequiredString(entry, "description", hero.description)
        || !readRequiredPositiveNumber(entry, "baseAttack", hero.baseAttack)
        || !readRequiredPositiveNumber(entry, "attackUpgradeBaseGain", hero.attackUpgradeBaseGain)
        || !readRequiredPositiveNumber(entry, "firstUpgradeGoldCost", hero.firstUpgradeGoldCost)
        || !readRequiredPositiveNumber(entry, "upgradeCostMultiplier", hero.upgradeCostMultiplier)
        || !readRequiredPositiveNumber(entry, "baseAttackIntervalSeconds", hero.baseAttackIntervalSeconds)
        || !readImageFileName(entry, "icon", hero.iconImageFile)
        || !readImageFileName(entry, "card", hero.cardImageFile)
        || !parseAttackLevelMultiplierRanges(entry, hero.attackLevelMultiplierRanges))
    {
        return false;
    }

    if (!entry.HasMember("baseHeroLevel") || !entry["baseHeroLevel"].IsInt()
        || entry["baseHeroLevel"].GetInt() < kMinimumHeroLevel)
    {
        cocos2d::log("[ConfigService] invalid baseHeroLevel for hero: %s", hero.id.c_str());
        return false;
    }

    hero.baseHeroLevel = entry["baseHeroLevel"].GetInt();

    if (!entry.HasMember("skills") || !entry["skills"].IsArray())
    {
        cocos2d::log("[ConfigService] missing skills array for hero: %s", hero.id.c_str());
        return false;
    }

    for (const rapidjson::Value& skillEntry : entry["skills"].GetArray())
    {
        HeroSkillConfig skill;
        if (!parseHeroSkill(skillEntry, skill))
        {
            return false;
        }

        hero.skills.push_back(skill);
    }

    return true;
}

/// 校验配置数组存在且非空。
/// 返回值：合法时返回 true。
bool hasNonEmptyArray(const rapidjson::Document& document, const char* key)
{
    if (!document.HasMember(key) || !document[key].IsArray() || document[key].Empty())
    {
        cocos2d::log("[ConfigService] missing or empty array: %s", key);
        return false;
    }

    return true;
}

}  // namespace

bool ConfigService::load()
{
    _bosses.clear();
    _heroes.clear();

    if (!_loadBosses() || !_loadHeroes())
    {
        // 任一配置失败都不保留半份数据，避免上层拿到不完整配置。
        _bosses.clear();
        _heroes.clear();
        return false;
    }

    return true;
}

const std::vector<BossConfig>& ConfigService::getBosses() const
{
    return _bosses;
}

const std::vector<HeroConfig>& ConfigService::getHeroes() const
{
    return _heroes;
}

bool ConfigService::_loadBosses()
{
    rapidjson::Document document;
    if (!parseConfigFile(kBossConfigPath, document) || !hasNonEmptyArray(document, "bosses"))
    {
        return false;
    }

    for (const rapidjson::Value& entry : document["bosses"].GetArray())
    {
        BossConfig boss;
        if (!parseBoss(entry, boss))
        {
            return false;
        }

        _bosses.push_back(boss);
    }

    return true;
}

bool ConfigService::_loadHeroes()
{
    rapidjson::Document document;
    if (!parseConfigFile(kHeroConfigPath, document) || !hasNonEmptyArray(document, "heroes"))
    {
        return false;
    }

    for (const rapidjson::Value& entry : document["heroes"].GetArray())
    {
        HeroConfig hero;
        if (!parseHero(entry, hero))
        {
            return false;
        }

        _heroes.push_back(hero);
    }

    return true;
}

}  // namespace DemonRealm
