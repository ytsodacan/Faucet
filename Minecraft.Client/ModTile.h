#pragma once
#include "Tile.h"
#include "Material.h"

// ============================================================================
// ModTile
// Tile's constructor and setters are all protected, so we need a thin
// subclass to expose them for use from the registry.
// ============================================================================

class ModTile : public Tile
{
public:
    ModTile(int id, Material* mat, bool solid = true)
        : Tile(id, mat, solid)
    {
    }

    // Expose all the protected setters as public passthroughs
    Tile* pubSetDestroyTime(float v)              { return setDestroyTime(v); }
    Tile* pubSetExplodeable(float v)              { return setExplodeable(v); }
    Tile* pubSetSoundType(const SoundType* s)     { return setSoundType(s); }
    Tile* pubSetLightBlock(int v)                 { return setLightBlock(v); }
    Tile* pubSetLightEmission(float f)            { return setLightEmission(f); }
    Tile* pubSetIconName(const std::wstring& n)   { return setIconName(n); }
};
