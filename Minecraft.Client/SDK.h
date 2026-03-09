#pragma once
#include "ModExport.h"
#include <string>
#include <vector>

// Forward declarations - Tells the compiler these exist without loading their files
class Minecraft;
class MinecraftServer;
class ServerLevel;
class MultiPlayerLevel;
class PlayerList;
class ServerPlayer;
class MultiplayerLocalPlayer;

namespace SDK {
    // Logging
    MODAPI void Log(const std::wstring& message);
    MODAPI void LogWarn(const std::wstring& message);
    MODAPI void LogError(const std::wstring& message);

    // Singletons
    MODAPI MinecraftServer* GetServer();
    MODAPI Minecraft* GetClient();

    // Levels
    MODAPI ServerLevel* GetServerLevel(int dimension = 0);
    MODAPI MultiPlayerLevel* GetClientLevel(int dimension = 0);

    // Players
    MODAPI PlayerList* GetPlayerList();
    // Use MultiplayerLocalPlayer (lowercase p) to match your Minecraft.h
    MODAPI MultiplayerLocalPlayer* GetLocalPlayer(int index);
    MODAPI MultiplayerLocalPlayer* GetLocalPlayer();

    // Messaging
    MODAPI void BroadcastMessage(const std::wstring& message);
    MODAPI void SendMessageToPlayer(const std::wstring& playerName, const std::wstring& message);

    // World Control
    MODAPI void ExecuteCommand(const std::wstring& command);
    MODAPI void SetTimeOfDay(__int64 time);
    MODAPI void SaveAll(bool force = false);

    // Rules
    MODAPI bool IsPvpAllowed();
    MODAPI void SetPvpAllowed(bool enabled);
    MODAPI bool IsFlightAllowed();
    MODAPI void SetFlightAllowed(bool enabled);
}