#include "stdafx.h"
#include "Registry.h"
#include "ModLoader.h"
#include "ModTile.h"             // ModTile  — exposes Tile's protected ctor/setters
#include "ModItem.h"             // ModItem  — exposes Item's protected ctor/setMaxDamage
#include "Material.h"            // Material::stone, Material::wood, etc.
#include "TileItem.h"            // TileItem(int id)
#include "HeavyTile.h"           // HeavyTile — falling blocks (sand/gravel style)
#include "HalfSlabTile.h"        // HalfSlabTile base
#include "StoneSlabTile.h"       // StoneSlabTile(int id, bool fullSize)
#include "WoodSlabTile.h"        // WoodSlabTile(int id, bool fullSize)
#include "StoneSlabTileItem.h"   // StoneSlabTileItem(int id, HalfSlabTile* half, HalfSlabTile* full, bool full)
#include "PickaxeItem.h"         // PickaxeItem(int id, const Item::Tier*)
#include "ShovelItem.h"          // ShovelItem(int id, const Item::Tier*)
#include "HatchetItem.h"         // HatchetItem(int id, const Item::Tier*)
#include "HoeItem.h"             // HoeItem(int id, const Item::Tier*)
#include "WeaponItem.h"          // WeaponItem(int id, const Item::Tier*)
#include "FurnaceRecipes.h"      // FurnaceRecipes::getInstance(), addFurnaceRecipy()
#include "ItemInstance.h"        // ItemInstance(int id, int count, int auxValue)
#include "./Common/UI/IUIScene_CreativeMenu.h"

// ---------------------------------------------------------------------------
// GameBridge — all real game calls live here
// ---------------------------------------------------------------------------

namespace GameBridge
{
    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    // Map our MaterialType enum -> game's Material* static pointer
    static Material* GetMaterial(MaterialType m)
    {
        switch (m)
        {
        case MaterialType::Stone:     return Material::stone;
        case MaterialType::Wood:      return Material::wood;
        case MaterialType::Cloth:     return Material::cloth;
        case MaterialType::Plant:     return Material::plant;
        case MaterialType::Dirt:      return Material::dirt;
        case MaterialType::Sand:      return Material::sand;
        case MaterialType::Glass:     return Material::glass;
        case MaterialType::Water:     return Material::water;
        case MaterialType::Lava:      return Material::lava;
        case MaterialType::Ice:       return Material::ice;
        case MaterialType::Metal:     return Material::metal;
        case MaterialType::Snow:      return Material::snow;
        case MaterialType::Clay:      return Material::clay;
        case MaterialType::Explosive: return Material::explosive;
        case MaterialType::Web:       return Material::web;
        case MaterialType::Air:
        default:                      return Material::stone; // safe fallback
        }
    }



    // Map our SoundType enum -> game's Tile::SoundType* static pointer
    static const Tile::SoundType* GetSound(::SoundType s)
    {
        switch (s)
        {
        case ::SoundType::Wood:   return Tile::SOUND_WOOD;
        case ::SoundType::Gravel: return Tile::SOUND_GRAVEL;
        case ::SoundType::Grass:  return Tile::SOUND_GRASS;
        case ::SoundType::Metal:  return Tile::SOUND_METAL;
        case ::SoundType::Glass:  return Tile::SOUND_GLASS;
        case ::SoundType::Cloth:  return Tile::SOUND_CLOTH;
        case ::SoundType::Sand:   return Tile::SOUND_SAND;
        case ::SoundType::Snow:   return Tile::SOUND_SNOW;
        case ::SoundType::Stone:
        default:                  return Tile::SOUND_STONE;
        }
    }

    // Map our ToolTier enum -> game's Item::Tier* static pointer
    static const Item::Tier* GetTier(ToolTier t)
    {
        switch (t)
        {
        case ToolTier::Wood:    return Item::Tier::WOOD;
        case ToolTier::Stone:   return Item::Tier::STONE;
        case ToolTier::Iron:    return Item::Tier::IRON;
        case ToolTier::Gold:    return Item::Tier::GOLD;
        case ToolTier::Diamond:
        default:                return Item::Tier::DIAMOND;
        }
    }

    // Description ID counter — starts above any vanilla IDs.
    // These integers index into the game's string table (Strings.h / .wds file).
    // If your string system uses a registration function, call it here instead
    // of (or in addition to) setting the description ID directly.
    static unsigned int s_nextDescId = 10000;
    static unsigned int AllocDescId() { return s_nextDescId++; }

    // Apply the common setters to any Tile subclass
    static void ApplyTileCommon(
        ModTile* t,
        float hardness, float resistance,
        ::SoundType sound,
        const wchar_t* icon,
        float lightEmission, int lightBlock,
        const wchar_t* displayName)
    {
        t->pubSetDestroyTime(hardness);
        t->pubSetExplodeable(resistance);
        t->pubSetSoundType(GetSound(sound));
        if (icon && icon[0])
            t->pubSetIconName(std::wstring(icon));
        t->pubSetLightEmission(lightEmission);
        t->pubSetLightBlock(lightBlock);
        if (displayName && displayName[0])
        {
            unsigned int descId = AllocDescId();
            // If your game has a string registration call, add it here, e.g.:
            //   Strings::registerString(descId, displayName);
            
            t->setDescriptionId(descId);
        }
    }

    // ------------------------------------------------------------------
    // Tiles (Blocks)
    // ------------------------------------------------------------------

    static bool CreateTile(
        int tileId,
        MaterialType material,
        float hardness, float resistance,
        ::SoundType sound,
        const wchar_t* icon,
        float lightEmission, int lightBlock,
        const wchar_t* displayName)
    {
        Material* mat = GetMaterial(material);

        ModTile* t = new ModTile(tileId, mat, true);
        ApplyTileCommon(t, hardness, resistance, sound, icon,
            lightEmission, lightBlock, displayName);

        // Create the matching TileItem so the block appears in inventory.
        // TileItem constructor param = tileId (NOT tileId - 256) — it does
        // the subtraction internally to produce Item::id = tileId.
        new TileItem(tileId);

        ModLoader::Get().Log(std::wstring(L"[Registry] Created Tile id=")
            + std::to_wstring(tileId) + L" icon=" + (icon ? icon : L"<none>"));
        return true;
    }

    static bool CreateFallingTile(
        int tileId,
        MaterialType material,
        float hardness, float resistance,
        ::SoundType sound,
        const wchar_t* icon,
        float lightEmission, int lightBlock,
        const wchar_t* displayName)
    {
        // HeavyTile constructor takes (int id, bool isSolidRender).
        // It doesn't take a Material* — it uses a hardcoded material internally.
        // We cast to ModTile only to reuse ApplyTileCommon; HeavyTile IS-A Tile
        // but we can't directly call protected setters on it, so we apply them
        // through a temporary ModTile to read the fields... actually, the safest
        // approach since HeavyTile sets up its own vtable is to create the
        // HeavyTile normally and set the fields we need via the public interface
        // where possible, and direct field writes where not.

        HeavyTile* t = new HeavyTile(tileId, true);

        // HeavyTile inherits Tile's protected setters; we can't call them from
        // here. Instead, write the fields directly — the layout is stable.
        // destroySpeed is at offset matching Tile's protected float destroySpeed.
        // If you have a ctor that accepts these params, prefer that.
        t->destroySpeed = hardness;
        t->explosionResistance = resistance;
        t->soundType = GetSound(sound);
        if (icon && icon[0])
            t->iconName = std::wstring(icon);
        // lightEmission and lightBlock are in the static arrays:
        Tile::lightEmission[tileId] = static_cast<int>(lightEmission * 15.0f);
        Tile::lightBlock[tileId] = lightBlock;

        if (displayName && displayName[0])
            t->setDescriptionId(AllocDescId());

        new TileItem(tileId);

        ModLoader::Get().Log(std::wstring(L"[Registry] Created FallingTile id=")
            + std::to_wstring(tileId));
        return true;
    }

    static bool CreateSlabPair(
        int halfId, int fullId,
        MaterialType material,
        float hardness, float resistance,
        ::SoundType sound,
        const wchar_t* icon,
        float lightEmission, int lightBlock,
        const wchar_t* displayName)
    {
        const bool wood = (material == MaterialType::Wood);
        unsigned int descId = 0;
        if (displayName && displayName[0])
            descId = AllocDescId();

        HalfSlabTile* half = nullptr;
        HalfSlabTile* full = nullptr;

        if (wood)
        {
            half = new WoodSlabTile(halfId, false);
            full = new WoodSlabTile(fullId, true);
        }
        else
        {
            half = new StoneSlabTile(halfId, false);
            full = new StoneSlabTile(fullId, true);
        }

        // Apply common properties to both halves via the Tile static arrays
        // and the public setDescriptionId (protected setters need subclass).
        // Use the static arrays for light values; iconName field is public-ish
        // through the wstring field in Tile.
        for (HalfSlabTile* slab : { half, full })
        {
            slab->destroySpeed = hardness;
            slab->explosionResistance = resistance;
            slab->soundType = GetSound(sound);
            if (icon && icon[0])
                slab->iconName = std::wstring(icon);
        }
        Tile::lightEmission[halfId] = Tile::lightEmission[fullId] =
            static_cast<int>(lightEmission * 15.0f);
        Tile::lightBlock[halfId] = Tile::lightBlock[fullId] = lightBlock;

        if (descId > 0)
        {
            half->setDescriptionId(descId);
            full->setDescriptionId(descId);
        }

        // StoneSlabTileItem handles both wood and stone slabs.
        // Constructor: StoneSlabTileItem(int itemId, HalfSlabTile* half, HalfSlabTile* full, bool isFullSlab)
        // itemId = halfId (the item represents the half-slab; double is placed in world only)
        new StoneSlabTileItem(halfId, half, full, false);

        ModLoader::Get().Log(std::wstring(L"[Registry] Created SlabPair half=")
            + std::to_wstring(halfId) + L" full=" + std::to_wstring(fullId));
        return true;
    }

    // ------------------------------------------------------------------
    // Items
    // ------------------------------------------------------------------

    static bool CreateItem(
        int itemId,
        int maxStackSize, int maxDamage,
        const wchar_t* icon,
        const wchar_t* displayName)
    {
        // Item constructor param = itemId - 256 (stores id = param + 256 internally,
        // then registers itself in Item::items[id]).
        ModItem* it = new ModItem(itemId - 256);

        it->setMaxStackSize(maxStackSize);
        if (maxDamage > 0)
            it->pubSetMaxDamage(maxDamage);
        if (icon && icon[0])
            it->setIconName(std::wstring(icon));
        if (displayName && displayName[0])
            it->setDescriptionId(AllocDescId());

        ModLoader::Get().Log(std::wstring(L"[Registry] Created Item id=")
            + std::to_wstring(itemId) + L" icon=" + (icon ? icon : L"<none>"));
        return true;
    }

    static bool CreateToolItem(
        int itemId,
        int toolKind,
        ToolTier tier,
        int maxDamage,
        float /*attackDamage*/,  // Tier::damage covers this; override field if needed
        int   /*harvestLevel*/,  // Tier::level covers this; override field if needed
        float /*destroySpeed*/,  // Tier::speed covers this; override field if needed
        const wchar_t* icon,
        const wchar_t* displayName)
    {
        const Item::Tier* t = GetTier(tier);
        const int ctorParam = itemId - 256;

        Item* it = nullptr;
        switch (toolKind)
        {
        case 0: it = new PickaxeItem(ctorParam, t); break;  // pickaxe
        case 1: it = new WeaponItem(ctorParam, t); break;  // sword
        case 2: it = new ShovelItem(ctorParam, t); break;  // shovel
        case 3: it = new HatchetItem(ctorParam, t); break;  // axe
        case 4: it = new HoeItem(ctorParam, t); break;  // hoe
        default:
            ModLoader::Get().Log(L"[Registry] CreateToolItem: unknown toolKind "
                + std::to_wstring(toolKind));
            return false;
        }

        // Tools never stack
        it->setMaxStackSize(1);
        if (maxDamage > 0)
        {
            // setMaxDamage is protected on Item — cast through ModItem isn't
            // possible once we have a base-class pointer from the ctor above.
            // Use the public setMaxDamage that DiggerItem/WeaponItem may expose,
            // or write the field directly. The field layout is:
            //   Item layout (from Item.h):
            //     int id              (public const)   +0x00 (after vtable)
            //     int maxStackSize    (protected)
            //     int maxDamage       (private)
            // Safest: each tool class likely inherits and exposes setMaxDamage —
            // if not, cast to ModItem is safe here because the actual most-derived
            // object IS a ModItem only for plain items. For tool subclasses use:
            it->setMaxDamage(maxDamage);
            // If you know the exact offset from your PDB/debugging, hardcode it.
            // A safe alternative: add setMaxDamage to DiggerItem.h as public.
        }

        if (icon && icon[0])
            it->setIconName(std::wstring(icon));
        if (displayName && displayName[0])
            it->setDescriptionId(AllocDescId());

        ModLoader::Get().Log(std::wstring(L"[Registry] Created ToolItem id=")
            + std::to_wstring(itemId) + L" kind=" + std::to_wstring(toolKind));
        return true;
    }

    // ------------------------------------------------------------------
    // Furnace recipe
    // ------------------------------------------------------------------

    static void AddFurnaceRecipe(int inputId, int outputId, float xp)
    {
        // addFurnaceRecipy (note: the method name has a typo in the original source)
        FurnaceRecipes::getInstance()->addFurnaceRecipy(
            inputId,
            new ItemInstance(outputId, 1, 0),
            xp);

        ModLoader::Get().Log(std::wstring(L"[Registry] Furnace recipe: ")
            + std::to_wstring(inputId) + L" -> " + std::to_wstring(outputId)
            + L" (" + std::to_wstring(xp) + L"xp)");
    }

    // ------------------------------------------------------------------
    // Creative inventory
    // ------------------------------------------------------------------

    // categoryGroups is a protected static on IUIScene_CreativeMenu.
    // A derived class can access it, so we use a thin accessor subclass.
    class CreativeAccessor : public IUIScene_CreativeMenu
    {
    public:
        static void AddItem(int numericId, int count, int auxValue, int groupIndex)
        {
            if (groupIndex < 0 || groupIndex >= eCreativeInventoryGroupsCount)
                return;
            categoryGroups[groupIndex].push_back(
                std::make_shared<ItemInstance>(numericId, count, auxValue));
        }
    };

    void AddToCreativeGroup(int numericId, int count, int auxValue, int groupIndex)
    {
        CreativeAccessor::AddItem(numericId, count, auxValue, groupIndex);
        ModLoader::Get().Log(std::wstring(L"[Registry] Added id=")
            + std::to_wstring(numericId) + L" to creative group "
            + std::to_wstring(groupIndex));
    }

} // namespace GameBridge


// ============================================================================
// ToolMaterialStore
// ============================================================================

ToolMaterialStore& ToolMaterialStore::Instance()
{
    static ToolMaterialStore inst;
    return inst;
}

void ToolMaterialStore::Register(const Identifier& id, const ToolMaterialDefinition& def)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_materials[id] = def;
}

bool ToolMaterialStore::TryGet(const Identifier& id, ToolMaterialDefinition& outDef) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it == m_materials.end()) return false;
    outDef = it->second;
    return true;
}


// ============================================================================
// Internal creative queue
// ============================================================================

namespace
{
    static std::vector<PendingCreativeItem> s_pendingCreative;
    static std::mutex s_creativeMutex;
}

namespace Registry {
    namespace Internal
    {
        void QueueCreativeItem(int numericId, int count, int auxValue, int groupIndex)
        {
            std::lock_guard<std::mutex> lock(s_creativeMutex);
            s_pendingCreative.push_back({ numericId, count, auxValue, groupIndex });
        }

        const std::vector<PendingCreativeItem>& GetPendingCreativeItems()
        {
            return s_pendingCreative;
        }

        void ClearPendingCreativeItems()
        {
            std::lock_guard<std::mutex> lock(s_creativeMutex);
            s_pendingCreative.clear();
        }
    }
}


// ============================================================================
// Registry::Block
// ============================================================================

namespace Registry {
    namespace Block
    {
        static RegisteredBlock DoRegisterBlock(
            const Identifier& id,
            const BlockProperties& props,
            bool falling)
        {
            RegisteredBlock result;
            result.id = id;

            int numericId = IdRegistry::Instance().Register(
                IdRegistry::Type::Block, id.ToStringA());

            if (numericId < 0) {
                ModLoader::Get().Log(L"[Registry] Block: no free IDs for " + id.ToString());
                return result;
            }

            bool ok = falling
                ? GameBridge::CreateFallingTile(numericId, props.material, props.hardness,
                    props.resistance, props.sound, props.icon.c_str(), props.lightEmission,
                    props.lightBlock, props.name.empty() ? nullptr : props.name.c_str())
                : GameBridge::CreateTile(numericId, props.material, props.hardness,
                    props.resistance, props.sound, props.icon.c_str(), props.lightEmission,
                    props.lightBlock, props.name.empty() ? nullptr : props.name.c_str());

            if (!ok) {
                ModLoader::Get().Log(L"[Registry] Block: CreateTile failed for " + id.ToString());
                return result;
            }

            if (props.creativeTab != CreativeTab::None)
                Internal::QueueCreativeItem(numericId, 1, 0, static_cast<int>(props.creativeTab));

            result.numericId = numericId;
            result.valid = true;

            ModLoader::Get().Log(L"[Registry] Registered block '" + id.ToString()
                + L"' -> id " + std::to_wstring(numericId));
            return result;
        }

        RegisteredBlock Register(const Identifier& id, const BlockProperties& props)
        {
            return DoRegisterBlock(id, props, false);
        }

        RegisteredBlock RegisterFalling(const Identifier& id, const BlockProperties& props)
        {
            return DoRegisterBlock(id, props, true);
        }

        RegisteredSlabBlock RegisterSlab(const Identifier& id, const BlockProperties& props)
        {
            RegisteredSlabBlock result;
            result.id = id;

            int halfId = IdRegistry::Instance().Register(IdRegistry::Type::Block, id.ToStringA());
            if (halfId < 0) {
                ModLoader::Get().Log(L"[Registry] Slab: no free ID for half " + id.ToString());
                return result;
            }

            Identifier doubleId(id.ns + L":" + id.path + L"_double");
            int fullId = IdRegistry::Instance().Register(
                IdRegistry::Type::Block, doubleId.ToStringA());
            if (fullId < 0) {
                ModLoader::Get().Log(L"[Registry] Slab: no free ID for double " + doubleId.ToString());
                return result;
            }

            bool ok = GameBridge::CreateSlabPair(halfId, fullId, props.material,
                props.hardness, props.resistance, props.sound, props.icon.c_str(),
                props.lightEmission, props.lightBlock,
                props.name.empty() ? nullptr : props.name.c_str());

            if (!ok) return result;

            if (props.creativeTab != CreativeTab::None)
                Internal::QueueCreativeItem(halfId, 1, 0, static_cast<int>(props.creativeTab));

            result.numericId = halfId;
            result.doubleId = doubleId;
            result.doubleNumericId = fullId;
            result.valid = true;

            ModLoader::Get().Log(L"[Registry] Registered slab '" + id.ToString()
                + L"' half=" + std::to_wstring(halfId)
                + L" double=" + std::to_wstring(fullId));
            return result;
        }

        int GetNumericId(const Identifier& id)
        {
            return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, id.ToStringA());
        }
    }
} // namespace Registry::Block
// ============================================================================

// ============================================================================
// Registry::Item
// ============================================================================

namespace Registry {
    namespace Item
    {
        void RegisterToolMaterial(const Identifier& id, const ToolMaterialDefinition& def)
        {
            ToolMaterialStore::Instance().Register(id, def);
            ModLoader::Get().Log(L"[Registry] Registered tool material " + id.ToString());
        }

        RegisteredItem Register(const Identifier& id, const ItemProperties& props)
        {
            RegisteredItem result;
            result.id = id;

            int numericId = IdRegistry::Instance().Register(
                IdRegistry::Type::Item, id.ToStringA());
            if (numericId < 0) {
                ModLoader::Get().Log(L"[Registry] Item: no free IDs for " + id.ToString());
                return result;
            }

            bool ok = GameBridge::CreateItem(numericId, props.maxStackSize, props.maxDamage,
                props.icon.empty() ? L"MISSING_ICON" : props.icon.c_str(),
                props.name.empty() ? nullptr : props.name.c_str());
            if (!ok) return result;

            if (props.creativeTab != CreativeTab::None)
                Internal::QueueCreativeItem(numericId, 1, 0, static_cast<int>(props.creativeTab));

            result.numericId = numericId;
            result.valid = true;
            ModLoader::Get().Log(L"[Registry] Registered item '" + id.ToString()
                + L"' -> id " + std::to_wstring(numericId));
            return result;
        }

        static RegisteredItem RegisterTool(
            const Identifier& id,
            const ItemProperties& props,
            ToolTier tier,
            int toolKind)
        {
            RegisteredItem result;
            result.id = id;

            int numericId = IdRegistry::Instance().Register(
                IdRegistry::Type::Item, id.ToStringA());
            if (numericId < 0) {
                ModLoader::Get().Log(L"[Registry] Tool: no free IDs for " + id.ToString());
                return result;
            }

            // Resolve tool material overrides if a custom material was set via
            // RegisterToolMaterial (keyed by the same id).
            ToolMaterialDefinition matDef;
            bool hasMat = ToolMaterialStore::Instance().TryGet(id, matDef);
            int   harvestLevel = hasMat ? matDef.harvestLevel : -1;
            float destroySpeed = hasMat ? matDef.destroySpeed : 0.0f;
            ToolTier effectiveTier = hasMat ? matDef.baseTier : tier;

            bool ok = GameBridge::CreateToolItem(
                numericId,
                toolKind,
                effectiveTier,
                props.maxDamage,
                props.attackDamage,
                harvestLevel,
                destroySpeed,
                props.icon.empty() ? L"MISSING_ICON" : props.icon.c_str(),
                props.name.empty() ? nullptr : props.name.c_str());
            if (!ok) return result;

            if (props.creativeTab != CreativeTab::None)
                Internal::QueueCreativeItem(numericId, 1, 0, static_cast<int>(props.creativeTab));

            result.numericId = numericId;
            result.valid = true;
            ModLoader::Get().Log(L"[Registry] Registered tool '" + id.ToString()
                + L"' kind=" + std::to_wstring(toolKind)
                + L" id=" + std::to_wstring(numericId));
            return result;
        }

        RegisteredItem RegisterPickaxe(const Identifier& id, const ItemProperties& props, ToolTier tier)
        {
            return RegisterTool(id, props, tier, 0);
        }

        RegisteredItem RegisterSword(const Identifier& id, const ItemProperties& props, ToolTier tier)
        {
            return RegisterTool(id, props, tier, 1);
        }

        RegisteredItem RegisterShovel(const Identifier& id, const ItemProperties& props, ToolTier tier)
        {
            return RegisterTool(id, props, tier, 2);
        }

        RegisteredItem RegisterAxe(const Identifier& id, const ItemProperties& props, ToolTier tier)
        {
            return RegisterTool(id, props, tier, 3);
        }

        RegisteredItem RegisterHoe(const Identifier& id, const ItemProperties& props, ToolTier tier)
        {
            return RegisterTool(id, props, tier, 4);
        }

        int GetNumericId(const Identifier& id)
        {
            return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, id.ToStringA());
        }
    }
} // namespace Registry::Item


// ============================================================================
// Registry::Entity
// ============================================================================

namespace Registry {
    namespace Entity
    {
        RegisteredEntity Register(const Identifier& id, const EntityDefinition& def)
        {
            RegisteredEntity result;
            result.id = id;

            int numericId = IdRegistry::Instance().Register(
                IdRegistry::Type::Entity, id.ToStringA());
            if (numericId < 0) {
                ModLoader::Get().Log(L"[Registry] Entity: no free IDs for " + id.ToString());
                return result;
            }

            // Entities in LCE are typically registered in EntityIO::staticCtor.
            // If your game supports dynamic entity registration, do it here.
            // TODO: call your entity factory if needed.

            result.numericId = numericId;
            result.valid = true;
            ModLoader::Get().Log(L"[Registry] Registered entity '" + id.ToString()
                + L"' -> id " + std::to_wstring(numericId));
            return result;
        }

        int GetNumericId(const Identifier& id)
        {
            return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Entity, id.ToStringA());
        }
    }
} // namespace Registry::Entity


// ============================================================================
// Registry::Recipe
// ============================================================================

namespace Registry {
    namespace Recipe
    {
        void AddFurnace(const Identifier& input, const Identifier& output, float xp)
        {
            int inputId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, input.ToStringA());
            if (inputId < 0)
                inputId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, input.ToStringA());

            int outputId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, output.ToStringA());
            if (outputId < 0)
                outputId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, output.ToStringA());

            if (inputId < 0) {
                ModLoader::Get().Log(L"[Registry] Furnace: unknown input " + input.ToString());
                return;
            }
            if (outputId < 0) {
                ModLoader::Get().Log(L"[Registry] Furnace: unknown output " + output.ToString());
                return;
            }

            GameBridge::AddFurnaceRecipe(inputId, outputId, xp);
            ModLoader::Get().Log(L"[Registry] Furnace recipe: "
                + input.ToString() + L" -> " + output.ToString()
                + L" (" + std::to_wstring(xp) + L"xp)");
        }

        void AddShaped(
            const Identifier& result,
            int resultCount,
            const std::vector<std::wstring>& pattern,
            const std::vector<ShapedKey>& keys)
        {
            // TODO: wire up to your game's CraftingManager or equivalent shaped recipe table.
            // Most LCE versions have a static shaped recipe list populated during staticCtor.
            // This stub logs the registration so you can see it's being called.
            ModLoader::Get().Log(L"[Registry] Shaped recipe -> " + std::to_wstring(resultCount)
                + L"x " + result.ToString() + L" (TODO: wire to game)");
        }
    }
} // namespace Registry::Recipe