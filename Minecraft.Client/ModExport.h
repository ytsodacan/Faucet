#pragma once

/**
 * @file ModExport.h
 * @brief Defines the MODAPI macro used to mark functions as mod-accessible.
 *
 * In your app project, add MODLOADER_HOST to your Preprocessor Definitions.
 * This causes every MODAPI-tagged function to be exported from the EXE and
 * included in the generated .lib file that you ship to modders.
 *
 * Modders link against that .lib and get full IDE autocomplete and docs on
 * every function you tagged — no function pointers, no casting required.
 *
 * @par Adding to your source
 * @code
 *   // In Player.h (or any header with functions you want to expose):
 *   #include "ModExport.h"
 *
 *   class Player {
 *   public:
 *       MODAPI float getHealth() const;
 *       MODAPI void  setHealth(float hp);
 *       // Un-tagged functions are completely invisible to mods
 *       void internalTick();
 *   };
 * @endcode
 */
#if defined(MODLOADER_HOST)
    #define MODAPI __declspec(dllexport)
#else
    #define MODAPI __declspec(dllimport)
#endif
