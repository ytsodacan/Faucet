#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

// ============================================================================
// IdRegistry
// Allocates numeric IDs for custom blocks, items, and entities.
// Vanilla IDs are pre-registered so lookups work for both.
//
// ID ranges (LCE):
//   Blocks  : 174-255  (vanilla uses 0-173)
//   Items   : 3000-31999 (vanilla uses 256-2267)
//   Entities: 1000-9999
// ============================================================================

class IdRegistry
{
public:
    enum class Type { Block, Item, Entity };

    static IdRegistry& Instance();

    // Register a new mod entry. Returns allocated ID, or -1 on failure.
    int Register(Type type, const std::string& namespacedId);

    // Lookup by string -> numeric. Returns -1 if not found.
    int GetNumericId(Type type, const std::string& namespacedId) const;

    // Lookup by numeric -> string. Returns "" if not found.
    std::string GetStringId(Type type, int numericId) const;

    // Pre-register a vanilla entry (called at startup for vanilla IDs).
    void RegisterVanilla(Type type, int numericId, const std::string& namespacedId);

    // Returns a snapshot of all numeric->string entries for a type.
    std::vector<std::pair<int, std::string>> GetEntries(Type type) const;

private:
    IdRegistry();

    struct RegistryData
    {
        std::unordered_map<std::string, int> stringToNum;
        std::unordered_map<int, std::string> numToString;
        int nextFreeId = 0;
        int maxId      = 0;
    };

    static constexpr int BLOCK_MOD_START  = 174;
    static constexpr int BLOCK_MAX        = 255;
    static constexpr int ITEM_MOD_START   = 3000;
    static constexpr int ITEM_MAX         = 31999;
    static constexpr int ENTITY_MOD_START = 1000;
    static constexpr int ENTITY_MAX       = 9999;

    RegistryData       m_registries[3];
    mutable std::mutex m_mutex;
};
