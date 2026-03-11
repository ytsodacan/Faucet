#pragma once
#include "Item.h"

// ============================================================================
// ModItem
// Item's constructor is protected, and setMaxDamage is protected.
// This thin subclass exposes both for the registry.
// ============================================================================

class ModItem : public Item
{
public:
    explicit ModItem(int id) : Item(id) {}

    // Expose protected setMaxDamage
    Item* pubSetMaxDamage(int dmg) { return setMaxDamage(dmg); }
};
