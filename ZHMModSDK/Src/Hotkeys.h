#pragma once

// Keyboard shortcuts that users can change from the SDK menu.
//
// Mods declare their shortcuts (RegisterHotkey) instead of hard-coding them in a game input
// binding; the chosen combination is stored in the mod's settings (section [hotkeys]) and can be
// changed in the "Keyboard shortcuts" window, opened with the SHORTCUTS button of the SDK menu bar.
// Keys are read with GetAsyncKeyState, only while the game window is in the foreground and the
// SDK menu does not have focus - like the game bindings, which the SDK disables while it has focus.

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class IPluginInterface;

struct HotkeyCombo {
    bool Ctrl = false;
    bool Alt = false;
    bool Shift = false;
    uint8_t Vk = 0;

    bool Empty() const { return Vk == 0; }

    bool operator==(const HotkeyCombo& p_Other) const {
        return Ctrl == p_Other.Ctrl && Alt == p_Other.Alt && Shift == p_Other.Shift && Vk == p_Other.Vk;
    }
};

class HotkeyManager {
public:
    static HotkeyCombo Parse(const std::string& p_Text);
    static std::string Serialize(const HotkeyCombo& p_Combo);
    static std::string Display(const HotkeyCombo& p_Combo);

    void NoteMod(IPluginInterface* p_Plugin, const std::string& p_Name);
    void RemovePlugin(IPluginInterface* p_Plugin);

    void Register(IPluginInterface* p_Plugin, const char* p_Id, const char* p_Label, const char* p_Default);
    bool IsPressed(IPluginInterface* p_Plugin, const char* p_Id);
    std::string Text(IPluginInterface* p_Plugin, const char* p_Id);

    void Show() { m_Open = true; }
    void Draw(bool p_HasFocus);

private:
    struct Entry {
        IPluginInterface* Plugin = nullptr;
        std::string Mod;
        std::string Id;
        std::string Label;
        HotkeyCombo Default;
        HotkeyCombo Combo;
        bool WasDown = false;
    };

    enum class CaptureTarget {
        None,
        Entry,
        ConsoleKey,
        UIToggleKey,
    };

    static bool KeyDown(int p_Vk);
    static bool IsModifierVk(int p_Vk);
    static bool ComboDown(const HotkeyCombo& p_Combo);
    static bool InputAllowed();
    static std::string ScanCodeName(uint8_t p_ScanCode);

    void BeginCapture(CaptureTarget p_Target, size_t p_Index);
    void UpdateCapture();
    void SetEntryCombo(size_t p_Index, const HotkeyCombo& p_Combo);
    void SaveEntry(const Entry& p_Entry);

    std::mutex m_Mutex;
    std::vector<Entry> m_Entries;
    std::unordered_map<IPluginInterface*, std::string> m_ModNames;

    bool m_Open = false;

    CaptureTarget m_CaptureTarget = CaptureTarget::None;
    size_t m_CaptureIndex = 0;

    // Frames to wait after a capture: otherwise ImGui keyboard navigation would
    // "click" the button with the chosen key (Space, Enter) and start a new capture.
    int m_CaptureCooldown = 0;
    std::array<bool, 256> m_CaptureBaseline {};
};
