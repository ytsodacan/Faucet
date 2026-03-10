#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

class ServerLevel;
class IMod;

// ============================================================================
// ModVersion / ModInfo
// ============================================================================

struct ModVersion {
    int major;
    int minor;
    int patch;
};

struct ModInfo {
    const char* id;
    const char* name;
    const char* author;
    const char* description;
    ModVersion  version;
};


// ============================================================================
// IMod — every mod DLL implements this
// ============================================================================

class IMod {
public:
    virtual ~IMod() = default;

    virtual const ModInfo* GetInfo() const = 0;

    /**
     * First call after CreateMod(). Game singletons may not exist yet.
     * Safe for: internal setup, logging.
     * NOT safe for: Minecraft::GetInstance(), MinecraftServer::getInstance().
     * @return false to abort this mod.
     */
    virtual bool OnLoad() = 0;

    /**
     * Called after Minecraft::init() completes.
     * All game APIs are safe from here.
     * @return false to abort this mod.
     */
    virtual bool OnInit() = 0;

    /**
     * Called each time MinecraftServer::setLevel() runs.
     * @param dimension  0=Overworld, -1=Nether, 1=End
     * @param level      Pointer to the loaded ServerLevel (null-check it).
     */
    virtual void OnLevelLoad() {}
    virtual void OnLevelUnload() {}

    /**
     * Called every server tick (~20/sec) from MinecraftServer::tick().
     * Keep this fast. Return false to unsubscribe from ticks.
     * @param deltaTime Seconds since last tick (~0.05).
     */
    virtual bool OnUpdate(float deltaTime) = 0;

    /**
     * Called on shutdown before the DLL is freed.
     */
    virtual void OnShutdown() = 0;
};


// ============================================================================
// LoadedMod — internal bookkeeping
// ============================================================================

struct LoadedMod {
    HMODULE     module   = nullptr;
    IMod*       instance = nullptr;
    std::string path;
    bool        healthy  = true;
};


// ============================================================================
// ModLoader
// ============================================================================

class ModLoader {
public:
    static ModLoader& Get();

    ModLoader(const ModLoader&)            = delete;
    ModLoader& operator=(const ModLoader&) = delete;

    // ------------------------------------------------------------------
    // Wire these four calls into your existing source files.
    // See ModLoader.cpp for exactly where each one goes.
    // ------------------------------------------------------------------

    bool m_levelLoaded = false;

    void Initialize();
    void NotifyInit();
    void OnLevelLoad();
    void OnLevelUnload();
    void NotifyUpdate(float deltaTime);
    void Shutdown();

    // ------------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------------

    IMod*                          FindMod(const std::string& id) const;
    const std::vector<LoadedMod>&  GetMods()        const { return m_mods; }
    bool                           IsInitialized()  const { return m_initialized; }

    void Log(const std::wstring& msg);
    void Log(const std::string&  msg);

private:
    ModLoader()  = default;
    ~ModLoader() = default;

    void         ScanAndLoadMods(const std::wstring& modsDir);
    bool         LoadOneMod(const std::wstring& dllPath);
    void         UnloadOneMod(LoadedMod& mod);
    std::wstring GetModsDirectory() const;
    void         OpenLogFile();

    std::vector<LoadedMod> m_mods;
    HANDLE                 m_logFile     = INVALID_HANDLE_VALUE;
    bool                   m_initialized = false;
};