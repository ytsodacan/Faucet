# Faucet
**A mod loader for Minecraft: Legacy Console Edition**

Faucet lets you load custom DLL mods into Minecraft Legacy Console Edition using a custom fork of the game. Mods are written in C++ using the FaucetSDK and placed in the game's `mods\` folder.

---

## Requirements For Modding

- [Faucet (custom LCE fork)](https://github.com/ytsodacan/Faucet) — the modified game executable required to run mods
- [FaucetSDK](https://github.com/ytsodacan/FaucetSDK/tree/main) — headers, lib, and example mod for building your own mods
- Visual Studio 2022 with the **Desktop development with C++** workload
- Windows x64

---

## Installation

1. Download and set up the Faucet fork from the link above.
2. Place any mod `.dll` files into the `mods\` folder in your game directory.
3. Launch the game. Mods are loaded automatically at startup and logged to `mods\modloader.log`.

---

## Creating a Mod

### Project Setup

1. In Visual Studio 2022, create a new **Dynamic-Link Library (DLL)** project targeting **x64**.
2. Download the [FaucetSDK](https://github.com/ytsodacan/FaucetSDK/tree/main) and copy these files into your project folder:
   - `IMod.h`
   - `SDK.h`
   - `ModExport.h`
   - `Minecraft.Client.lib`
3. In your project properties:
   - **C/C++ → Additional Include Directories**: add `$(ProjectDir)`
   - **Linker → Input → Additional Dependencies**: add `Minecraft.Client.lib`
   - **Linker → General → Additional Library Directories**: add `$(ProjectDir)`
4. Remove `dllmain.cpp` from the project (right-click → Remove in Solution Explorer).

### Mod Structure

Every mod must implement the `IMod` interface and export a `CreateMod()` function:

```cpp
#include "IMod.h"
#include "SDK.h"

class MyMod final : public IMod {
public:
    const ModInfo* GetInfo() const override {
        static const ModInfo info{
            "com.yourname.mymod",   // Unique mod ID
            "My Mod",               // Display name
            "YourName",             // Author
            "Description here.",    // Description
            { 1, 0, 0 }             // Version
        };
        return &info;
    }

    bool OnLoad()     override { SDK::Log(L"MyMod: loaded");  return true; }
    bool OnInit()     override { SDK::Log(L"MyMod: init");    return true; }
    bool OnUpdate(float dt) override { return true; }
    void OnShutdown() override { SDK::Log(L"MyMod: shutdown"); }
};

extern "C" __declspec(dllexport) IMod* CreateMod() {
    return new MyMod();
}
```

Build the project and copy the output `.dll` into your `mods\` folder.

---

## SDK Reference

Include `SDK.h` in your mod to access all game systems.

### Logging

```cpp
SDK::Log(L"Hello from my mod!");
SDK::LogWarn(L"Something seems off.");
SDK::LogError(L"Something went wrong.");
```

All output is written to `mods\modloader.log`.

### Getting Game Objects

```cpp
Minecraft*       client = SDK::GetClient();
MinecraftServer* server = SDK::GetServer();
ServerLevel*     level  = SDK::GetServerLevel(0);  // 0=Overworld, -1=Nether, 1=End
PlayerList*      players = SDK::GetPlayerList();
MultiplayerLocalPlayer* localPlayer = SDK::GetLocalPlayer();
```

### Player

```cpp
auto* player = SDK::GetLocalPlayer();
if (player) {
    float hp = player->getHealth();     // 0.0–20.0
    player->setHealth(20.0f);           // Full heal
    player->setFlying(true);
    wstring name = player->getName();
}
```

### Server Players

```cpp
PlayerList* list = SDK::GetPlayerList();
if (list) {
    for (auto& sp : list->players)
        sp->sendMessage(L"Hello!");

    auto steve = list->getPlayer(L"Steve");
    if (steve) steve->disconnect();
}
```

### Messaging

```cpp
SDK::BroadcastMessage(L"Server restarting soon!");
SDK::SendMessageToPlayer(L"Steve", L"You've been warned.");
```

### Server Control

```cpp
SDK::ExecuteCommand(L"time set day");
SDK::SetTimeOfDay(6000);   // 0=dawn, 6000=noon, 12000=dusk, 18000=midnight
SDK::SaveAll();
SDK::SetPvpAllowed(false);
SDK::SetFlightAllowed(true);
```

### World / Level

```cpp
ServerLevel* level = SDK::GetServerLevel(0);
if (level) {
    level->explode(nullptr, 0, 64, 0, 4.0f, false, true);
    level->sendParticles(L"flame", x, y, z, 10);
    level->save(true, nullptr);
}
```

---

## Notes

- The mod loader calls `OnShutdown` each time a world unloads, not only on game exit. Design your mod accordingly if you need persistent state across world loads.
- Logs are written to `mods\modloader.log` — check here first if your mod isn't behaving as expected.
- `SDK::GetLocalPlayer()` returns `nullptr` when no world is loaded. Always null-check before use.

---

## Links

- **Faucet (game fork)**: https://github.com/ytsodacan/Faucet
- **FaucetSDK (mod tools)**: https://github.com/ytsodacan/FaucetSDK/tree/main
