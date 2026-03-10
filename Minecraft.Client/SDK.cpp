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
// Player Pos
// ============================================================================

double SDK::GetPlayerX(int index) {
    MultiplayerLocalPlayer* p = GetLocalPlayer(index);
    return p ? p->x : 0.0;
}

double SDK::GetPlayerY(int index) {
    MultiplayerLocalPlayer* p = GetLocalPlayer(index);
    return p ? p->y : 0.0;
}

double SDK::GetPlayerZ(int index) {
    MultiplayerLocalPlayer* p = GetLocalPlayer(index);
    return p ? p->z : 0.0;
}

PlayerPos SDK::GetPlayerPos(int index) {
    MultiplayerLocalPlayer* p = GetLocalPlayer(index);
    if (!p) return { 0.0, 0.0, 0.0 };
    return { p->x, p->y, p->z };
}

// ============================================================================
// Player Stats
// ============================================================================

float SDK::GetPlayerHealth(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->getHealth() : 0.0f;
}

void SDK::SetPlayerHealth(float health, int index) {
    auto* p = GetLocalPlayer(index);
    if (!p) return;
    if (health < 0.0f) health = 0.0f;
    if (health > p->getMaxHealth()) health = p->getMaxHealth();
    p->setHealth(health);
}

float SDK::GetPlayerMaxHealth(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->getMaxHealth() : 20.0f;
}

bool SDK::IsPlayerAlive(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->getHealth() > 0.0f : false;
}

// ============================================================================
// Player State
// ============================================================================

void SDK::SetPlayerFlying(bool flying, int index) {
    auto* p = GetLocalPlayer(index);
    if (p) p->abilities.flying = flying;
}

bool SDK::IsPlayerFlying(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->abilities.flying : false;
}

bool SDK::IsPlayerSprinting(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->isSprinting() : false;
}

bool SDK::IsPlayerSneaking(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->isSneaking() : false;
}

bool SDK::IsPlayerOnGround(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->onGround : false;
}

// ============================================================================
// Player Info
// ============================================================================

std::wstring SDK::GetPlayerName(int index) {
    auto* p = GetLocalPlayer(index);
    return p ? p->name : L"";
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
// World / Level
// ============================================================================

void SDK::ExplodeAt(double x, double y, double z, float radius, bool fire, bool destroyBlocks, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->explode(nullptr, x, y, z, radius, fire, destroyBlocks);
}

void SDK::SendParticles(const std::wstring& name, double x, double y, double z, int count, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->sendParticles(name, x, y, z, count);
}

void SDK::SendParticlesEx(const std::wstring& name, double x, double y, double z, int count, double xDist, double yDist, double zDist, double speed, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->sendParticles(name, x, y, z, count, xDist, yDist, zDist, speed);
}

void SDK::SaveLevel(bool force, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->save(force, nullptr);
}

bool SDK::IsLevelLoaded(int dimension) {
    return GetServerLevel(dimension) != nullptr;
}

void SDK::SetLevelTime(__int64 time, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->setTimeAndAdjustTileTicks(time);
}

__int64 SDK::GetLevelTime(int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    return level ? level->getGameTime() : 0LL;
}

void SDK::QueueTileUpdate(int x, int y, int z, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->queueSendTileUpdate(x, y, z);
}

void SDK::AddTickNextTick(int x, int y, int z, int tileId, int tickDelay, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    if (level) level->addToTickNextTick(x, y, z, tileId, tickDelay);
}

bool SDK::MayPlayerInteract(int index, int x, int y, int z, int content, int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    auto* p = GetLocalPlayer(index);
    if (!level || !p) return false;
    return level->mayInteract(std::shared_ptr<Player>(p, [](Player*) {}), x, y, z, content);
}

// ============================================================================
// Entity Limits
// ============================================================================

int SDK::GetPrimedTntCount(int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    return level ? level->m_primedTntCount : 0;
}

int SDK::GetFallingTileCount(int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    return level ? level->m_fallingTileCount : 0;
}

bool SDK::CanSpawnTnt(int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    return level ? level->newPrimedTntAllowed() : false;
}

bool SDK::CanSpawnFallingTile(int dimension) {
    ServerLevel* level = GetServerLevel(dimension);
    return level ? level->newFallingTileAllowed() : false;
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