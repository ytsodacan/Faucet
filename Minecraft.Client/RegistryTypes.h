#pragma once
#include <string>
#include <stdexcept>

// ============================================================================
// Identifier — "namespace:path" (e.g. "minecraft:stone", "mymod:ruby_ore")
// ============================================================================

struct Identifier
{
    std::wstring ns;
    std::wstring path;

    Identifier() = default;

    // Construct from wide "ns:path" string
    explicit Identifier(const std::wstring& id)
    {
        auto colon = id.find(L':');
        if (colon == std::wstring::npos) {
            ns   = L"minecraft";
            path = id;
        } else {
            ns   = id.substr(0, colon);
            path = id.substr(colon + 1);
        }
    }

    // Construct from narrow "ns:path" string
    explicit Identifier(const std::string& id)
    {
        auto colon = id.find(':');
        if (colon == std::string::npos) {
            ns   = L"minecraft";
            path = std::wstring(id.begin(), id.end());
        } else {
            std::string nsPart   = id.substr(0, colon);
            std::string pathPart = id.substr(colon + 1);
            ns   = std::wstring(nsPart.begin(),   nsPart.end());
            path = std::wstring(pathPart.begin(), pathPart.end());
        }
    }

    Identifier(const wchar_t* nsStr, const wchar_t* pathStr)
        : ns(nsStr), path(pathStr) {}

    std::wstring ToString()  const { return ns + L":" + path; }
    std::string  ToStringA() const
    {
        std::wstring w = ToString();
        return std::string(w.begin(), w.end());
    }

    bool operator==(const Identifier& o) const { return ns == o.ns && path == o.path; }
    bool operator!=(const Identifier& o) const { return !(*this == o); }
};

// ============================================================================
// CreativeTab — matches the game's internal group indices
// ============================================================================

enum class CreativeTab : int
{
    None           = -1,
    BuildingBlocks =  0,
    Decoration     =  1,
    Redstone       =  2,
    Transport      =  3,
    Materials      =  4,
    Food           =  5,
    ToolsAndWeapons=  6,
    Brewing        =  7,
    Misc           = 12
};

// ============================================================================
// Block enums
// ============================================================================

enum class MaterialType : int
{
    Air       =  0,
    Stone     =  1,
    Wood      =  2,
    Cloth     =  3,
    Plant     =  4,
    Dirt      =  5,
    Sand      =  6,
    Glass     =  7,
    Water     =  8,
    Lava      =  9,
    Ice       = 10,
    Metal     = 11,
    Snow      = 12,
    Clay      = 13,
    Explosive = 14,
    Web       = 15
};

enum class SoundType : int
{
    None   = 0,
    Stone  = 1,
    Wood   = 2,
    Gravel = 3,
    Grass  = 4,
    Metal  = 5,
    Glass  = 6,
    Cloth  = 7,
    Sand   = 8,
    Snow   = 9
};

enum class ToolType : int
{
    None    = 0,
    Pickaxe = 1,
    Axe     = 2,
    Shovel  = 3
};

// ============================================================================
// Item enums
// ============================================================================

enum class ToolTier : int
{
    Wood    = 0,
    Stone   = 1,
    Iron    = 2,
    Diamond = 3,
    Gold    = 4
};

// ============================================================================
// BlockProperties — fluent builder
// ============================================================================

struct BlockProperties
{
    MaterialType material     = MaterialType::Stone;
    float        hardness     = 1.0f;
    float        resistance   = 5.0f;
    SoundType    sound        = SoundType::Stone;
    std::wstring icon         = L"stone";
    float        lightEmission= 0.0f;
    int          lightBlock   = 255;
    CreativeTab  creativeTab  = CreativeTab::None;
    std::wstring name;
    int          harvestLevel = -1;
    ToolType     requiredTool = ToolType::None;
    bool         acceptsRedstone = false;

    BlockProperties& Material(MaterialType m)    { material = m; return *this; }
    BlockProperties& Hardness(float h)           { hardness = h; return *this; }
    BlockProperties& Resistance(float r)         { resistance = r; return *this; }
    BlockProperties& Sound(SoundType s)          { sound = s; return *this; }
    BlockProperties& Icon(const wchar_t* i)      { icon = i; return *this; }
    BlockProperties& Icon(const std::wstring& i) { icon = i; return *this; }
    BlockProperties& LightLevel(float l)         { lightEmission = l; return *this; }
    BlockProperties& LightBlocking(int l)        { lightBlock = l; return *this; }
    BlockProperties& Indestructible()            { hardness = -1.0f; resistance = 6000000.0f; return *this; }
    BlockProperties& InCreativeTab(CreativeTab t){ creativeTab = t; return *this; }
    BlockProperties& Name(const wchar_t* n)      { name = n; return *this; }
    BlockProperties& Name(const std::wstring& n) { name = n; return *this; }
    BlockProperties& RequiredHarvestLevel(int l) { harvestLevel = l; return *this; }
    BlockProperties& RequiredTool(ToolType t)    { requiredTool = t; return *this; }
    BlockProperties& AcceptsRedstonePower(bool a = true) { acceptsRedstone = a; return *this; }
};

// ============================================================================
// ItemProperties — fluent builder
// ============================================================================

struct ItemProperties
{
    int          maxStackSize = 64;
    int          maxDamage    = 0;
    float        attackDamage = 0.0f;
    std::wstring icon;
    CreativeTab  creativeTab  = CreativeTab::None;
    std::wstring name;

    ItemProperties& MaxStackSize(int s)          { maxStackSize = s; return *this; }
    ItemProperties& MaxDamage(int d)             { maxDamage = d; maxStackSize = 1; return *this; }
    ItemProperties& AttackDamage(float d)        { attackDamage = d; return *this; }
    ItemProperties& Icon(const wchar_t* i)       { icon = i; return *this; }
    ItemProperties& Icon(const std::wstring& i)  { icon = i; return *this; }
    ItemProperties& InCreativeTab(CreativeTab t) { creativeTab = t; return *this; }
    ItemProperties& Name(const wchar_t* n)       { name = n; return *this; }
    ItemProperties& Name(const std::wstring& n)  { name = n; return *this; }
};

// ============================================================================
// ToolMaterialDefinition
// ============================================================================

struct ToolMaterialDefinition
{
    ToolTier baseTier    = ToolTier::Diamond;
    int      harvestLevel= 3;
    float    destroySpeed= 8.0f;

    ToolMaterialDefinition& BaseTier(ToolTier t)      { baseTier = t; return *this; }
    ToolMaterialDefinition& HarvestLevel(int l)       { harvestLevel = l; return *this; }
    ToolMaterialDefinition& DestroySpeed(float s)     { destroySpeed = s; return *this; }
};

// ============================================================================
// EntityDefinition — fluent builder
// ============================================================================

struct EntityDefinition
{
    float width         = 0.6f;
    float height        = 1.8f;
    int   trackingRange = 80;

    EntityDefinition& Width(float w)         { width = w; return *this; }
    EntityDefinition& Height(float h)        { height = h; return *this; }
    EntityDefinition& TrackingRange(int r)   { trackingRange = r; return *this; }
    EntityDefinition& Size(float w, float h) { width = w; height = h; return *this; }
};

// ============================================================================
// Registration handles returned to mods
// ============================================================================

struct RegisteredBlock
{
    Identifier id;
    int        numericId = -1;
    bool       valid     = false;
};

struct RegisteredSlabBlock
{
    Identifier id;
    int        numericId       = -1;
    Identifier doubleId;
    int        doubleNumericId = -1;
    bool       valid           = false;
};

struct RegisteredItem
{
    Identifier id;
    int        numericId = -1;
    bool       valid     = false;
};

struct RegisteredEntity
{
    Identifier id;
    int        numericId = -1;
    bool       valid     = false;
};
