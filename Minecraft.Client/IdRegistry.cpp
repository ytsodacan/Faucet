#include "stdafx.h"
#include "IdRegistry.h"

IdRegistry& IdRegistry::Instance()
{
    static IdRegistry instance;
    return instance;
}

IdRegistry::IdRegistry()
{
    m_registries[static_cast<int>(Type::Block)].nextFreeId  = BLOCK_MOD_START;
    m_registries[static_cast<int>(Type::Block)].maxId       = BLOCK_MAX;
    m_registries[static_cast<int>(Type::Item)].nextFreeId   = ITEM_MOD_START;
    m_registries[static_cast<int>(Type::Item)].maxId        = ITEM_MAX;
    m_registries[static_cast<int>(Type::Entity)].nextFreeId = ENTITY_MOD_START;
    m_registries[static_cast<int>(Type::Entity)].maxId      = ENTITY_MAX;

    // ---- Vanilla Blocks ----
    struct VEntry { int id; const char* name; };
    static const VEntry kVanillaBlocks[] = {
        {0,"minecraft:air"},{1,"minecraft:stone"},{2,"minecraft:grass"},
        {3,"minecraft:dirt"},{4,"minecraft:cobblestone"},{5,"minecraft:planks"},
        {7,"minecraft:bedrock"},{8,"minecraft:flowing_water"},{9,"minecraft:water"},
        {10,"minecraft:flowing_lava"},{11,"minecraft:lava"},{12,"minecraft:sand"},
        {13,"minecraft:gravel"},{14,"minecraft:gold_ore"},{15,"minecraft:iron_ore"},
        {16,"minecraft:coal_ore"},{17,"minecraft:log"},{18,"minecraft:leaves"},
        {20,"minecraft:glass"},{21,"minecraft:lapis_ore"},{24,"minecraft:sandstone"},
        {41,"minecraft:gold_block"},{42,"minecraft:iron_block"},
        {45,"minecraft:brick_block"},{46,"minecraft:tnt"},{49,"minecraft:obsidian"},
        {54,"minecraft:chest"},{56,"minecraft:diamond_ore"},{57,"minecraft:diamond_block"},
        {61,"minecraft:furnace"},{73,"minecraft:redstone_ore"},{74,"minecraft:lit_redstone_ore"},
        {79,"minecraft:ice"},{80,"minecraft:snow"},{87,"minecraft:netherrack"},
        {89,"minecraft:glowstone"},{98,"minecraft:stonebrick"},{121,"minecraft:end_stone"},
        {123,"minecraft:redstone_lamp"},{152,"minecraft:redstone_block"},
        {153,"minecraft:quartz_ore"},{155,"minecraft:quartz_block"},
        {-1, nullptr}
    };
    for (int i = 0; kVanillaBlocks[i].name; ++i)
        RegisterVanilla(Type::Block, kVanillaBlocks[i].id, kVanillaBlocks[i].name);

    // ---- Vanilla Items ----
    static const VEntry kVanillaItems[] = {
        {256,"minecraft:iron_shovel"},{257,"minecraft:iron_pickaxe"},{258,"minecraft:iron_axe"},
        {260,"minecraft:apple"},{263,"minecraft:coal"},{264,"minecraft:diamond"},
        {265,"minecraft:iron_ingot"},{266,"minecraft:gold_ingot"},
        {267,"minecraft:iron_sword"},{268,"minecraft:wooden_sword"},
        {276,"minecraft:diamond_sword"},{277,"minecraft:diamond_shovel"},
        {278,"minecraft:diamond_pickaxe"},{279,"minecraft:diamond_axe"},
        {280,"minecraft:stick"},{287,"minecraft:string"},{289,"minecraft:gunpowder"},
        {296,"minecraft:wheat"},{331,"minecraft:redstone"},{340,"minecraft:book"},
        {344,"minecraft:egg"},{352,"minecraft:bone"},{388,"minecraft:emerald"},
        {-1, nullptr}
    };
    for (int i = 0; kVanillaItems[i].name; ++i)
        RegisterVanilla(Type::Item, kVanillaItems[i].id, kVanillaItems[i].name);

    // ---- Vanilla Entities ----
    static const VEntry kVanillaEntities[] = {
        {10,"minecraft:arrow"},{11,"minecraft:snowball"},{12,"minecraft:fireball"},
        {13,"minecraft:small_fireball"},{14,"minecraft:ender_pearl"},
        {19,"minecraft:wither_skull"},{20,"minecraft:primed_tnt"},
        {21,"minecraft:falling_block"},{50,"minecraft:creeper"},
        {51,"minecraft:skeleton"},{54,"minecraft:zombie"},{58,"minecraft:enderman"},
        {61,"minecraft:blaze"},{90,"minecraft:pig"},{91,"minecraft:sheep"},
        {92,"minecraft:cow"},{93,"minecraft:chicken"},
        {-1, nullptr}
    };
    for (int i = 0; kVanillaEntities[i].name; ++i)
        RegisterVanilla(Type::Entity, kVanillaEntities[i].id, kVanillaEntities[i].name);
}

int IdRegistry::Register(Type type, const std::string& namespacedId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& reg = m_registries[static_cast<int>(type)];

    // Already registered?
    auto it = reg.stringToNum.find(namespacedId);
    if (it != reg.stringToNum.end())
        return it->second;

    if (reg.nextFreeId > reg.maxId)
        return -1;

    // Skip over any IDs already taken by vanilla pre-registration
    while (reg.numToString.count(reg.nextFreeId) && reg.nextFreeId <= reg.maxId)
        ++reg.nextFreeId;

    if (reg.nextFreeId > reg.maxId)
        return -1;

    int id = reg.nextFreeId++;
    reg.stringToNum[namespacedId] = id;
    reg.numToString[id]           = namespacedId;
    return id;
}

int IdRegistry::GetNumericId(Type type, const std::string& namespacedId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& reg = m_registries[static_cast<int>(type)];
    auto it = reg.stringToNum.find(namespacedId);
    return (it != reg.stringToNum.end()) ? it->second : -1;
}

std::string IdRegistry::GetStringId(Type type, int numericId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& reg = m_registries[static_cast<int>(type)];
    auto it = reg.numToString.find(numericId);
    return (it != reg.numToString.end()) ? it->second : "";
}

void IdRegistry::RegisterVanilla(Type type, int numericId, const std::string& namespacedId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& reg = m_registries[static_cast<int>(type)];
    reg.stringToNum[namespacedId] = numericId;
    reg.numToString[numericId]    = namespacedId;
}

std::vector<std::pair<int, std::string>> IdRegistry::GetEntries(Type type) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& reg = m_registries[static_cast<int>(type)];
    std::vector<std::pair<int, std::string>> out;
    out.reserve(reg.numToString.size());
    for (const auto& kv : reg.numToString)
        out.push_back(kv);
    return out;
}
