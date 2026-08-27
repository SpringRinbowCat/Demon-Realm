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

/// 解析单个技能配置条目。
/// 返回值：字段齐全且解锁等级合法时返回 true。
bool parseHeroSkill(const rapidjson::Value& entry, HeroSkillConfig& skill)
{
    if (!entry.IsObject())
    {
        cocos2d::log("[ConfigService] skill entry is not an object");
        return false;
    }

    if (!readRequiredString(entry, "id", skill.id)
        || !readRequiredString(entry, "displayName", skill.displayName))
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
        || !readRequiredPositiveNumber(entry, "baseAttack", hero.baseAttack)
        || !readRequiredPositiveNumber(entry, "baseAttackIntervalSeconds", hero.baseAttackIntervalSeconds)
        || !readImageFileName(entry, "icon", hero.iconImageFile)
        || !readImageFileName(entry, "card", hero.cardImageFile))
    {
        return false;
    }

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
