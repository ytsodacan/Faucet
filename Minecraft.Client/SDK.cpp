#include "stdafx.h"
#include "SDK.h"
#include "ModLoader.h"

#include "Minecraft.h"
#include "MinecraftServer.h"
#include "ServerLevel.h"
#include "MultiPlayerLevel.h"
#include "PlayerList.h"
#include "ServerPlayer.h"
#include "MultiplayerLocalPlayer.h"

#ifndef XUSER_MAX_COUNT
#define XUSER_MAX_COUNT 4
#endif

// ============================================================================
// Logging
// ============================================================================

void SDK::Log(const std::wstring& message) {
    ModLoader::Get().Log(message);
}

void SDK::LogWarn(const std::wstring& message) {
    ModLoader::Get().Log(L"[WARN] " + message);
}

void SDK::LogError(const std::wstring& message) {
    ModLoader::Get().Log(L"[ERROR] " + message);
}

// ============================================================================
// Singletons
// ============================================================================

MinecraftServer* SDK::GetServer() {
    return MinecraftServer::getInstance(); // Lowercase g for Server
}

Minecraft* SDK::GetClient() {
    return Minecraft::GetInstance(); // Capital G for Client
}

// ============================================================================
// Level Access
// ============================================================================

ServerLevel* SDK::GetServerLevel(int dimension) {
    MinecraftServer* s = GetServer();
    return s ? s->getLevel(dimension) : nullptr;
}

MultiPlayerLevel* SDK::GetClientLevel(int dimension) {
    Minecraft* mc = GetClient();
    if (!mc) return nullptr;
    return mc->getLevel(dimension);
}

// ============================================================================
// Player Access (LCE Split-screen aware)
// ============================================================================

PlayerList* SDK::GetPlayerList() {
    MinecraftServer* s = GetServer();
    return s ? s->getPlayers() : nullptr;
}

MultiplayerLocalPlayer* SDK::GetLocalPlayer(int index) {
    Minecraft* mc = GetClient();
    if (!mc) return nullptr;

    if (index < 0 || index >= XUSER_MAX_COUNT) return nullptr;

    // In LCE, localplayers is an array of shared_ptr. Use .get() for the raw pointer.
    return mc->localplayers[index].get();
}

MultiplayerLocalPlayer* SDK::GetLocalPlayer() {
    Minecraft* mc = GetClient();
    if (!mc) return nullptr;

    // Return the player currently in focus
    int currentIdx = mc->getLocalPlayerIdx();
    return mc->localplayers[currentIdx].get();
}

// ============================================================================
// Messaging
// ============================================================================

void SDK::BroadcastMessage(const std::wstring& message) {
    PlayerList* list = GetPlayerList();
    if (list) list->sendMessage(L"", message);
}

void SDK::SendMessageToPlayer(const std::wstring& playerName, const std::wstring& message) {
    PlayerList* list = GetPlayerList();
    if (list) list->sendMessage(playerName, message);
}

// ============================================================================
// Server Control
// ============================================================================

void SDK::ExecuteCommand(const std::wstring& command) {
    MinecraftServer* s = GetServer();
    if (s) s->handleConsoleInput(command, nullptr);
}

void SDK::SetTimeOfDay(__int64 time) {
    // Some LCE versions use a static call, others use the instance
    MinecraftServer::SetTimeOfDay(time);
}

void SDK::SaveAll(bool force) {
    MinecraftServer* s = GetServer();
    if (!s) return;

    for (int dim : { 0, -1, 1 }) {
        ServerLevel* level = s->getLevel(dim);
        if (level) level->save(force, nullptr);
    }

    PlayerList* list = s->getPlayers();
    if (list) list->saveAll(nullptr);
}

// ============================================================================
// Game Rules
// ============================================================================

bool SDK::IsPvpAllowed() {
    MinecraftServer* s = GetServer();
    return s ? s->isPvpAllowed() : false;
}

void SDK::SetPvpAllowed(bool enabled) {
    MinecraftServer* s = GetServer();
    if (s) s->setPvpAllowed(enabled);
}

bool SDK::IsFlightAllowed() {
    MinecraftServer* s = GetServer();
    return s ? s->isFlightAllowed() : false;
}

void SDK::SetFlightAllowed(bool enabled) {
    MinecraftServer* s = GetServer();
    if (s) s->setFlightAllowed(enabled);
}