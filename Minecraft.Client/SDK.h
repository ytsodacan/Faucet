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

struct PlayerPos {
    double x, y, z;
};

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

    // Player Pos
    MODAPI double GetPlayerX(int index = 0);
    MODAPI double GetPlayerY(int index = 0);
    MODAPI double GetPlayerZ(int index = 0);
    MODAPI PlayerPos GetPlayerPos(int index = 0);

    // Player Stats
    MODAPI float GetPlayerHealth(int index = 0);
    MODAPI void  SetPlayerHealth(float health, int index = 0);
    MODAPI float GetPlayerMaxHealth(int index = 0);
    MODAPI bool  IsPlayerAlive(int index = 0);

    // Player State
    MODAPI void  SetPlayerFlying(bool flying, int index = 0);
    MODAPI bool  IsPlayerFlying(int index = 0);
    MODAPI bool  IsPlayerSprinting(int index = 0);
    MODAPI bool  IsPlayerSneaking(int index = 0);
    MODAPI bool  IsPlayerOnGround(int index = 0);

    // Player Info
    MODAPI std::wstring GetPlayerName(int index = 0);

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