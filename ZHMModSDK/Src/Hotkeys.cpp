#include "Hotkeys.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <format>
#include <map>
#include <unordered_set>

#include <imgui.h>

#include "IconsMaterialDesign.h"
#include "Logging.h"
#include "ModSDK.h"
#include "Rendering/Renderers/ImGuiRenderer.h"

namespace {
    // Names written to the .ini files: stable, independent of the keyboard layout.
    // Letters and digits are the keys that carry that character (Windows VK);
    // other keys are written as "0xBA".
    const std::map<uint8_t, const char*>& NamedKeys() {
        static const std::map<uint8_t, const char*> s_Keys = {
            { VK_MBUTTON, "MiddleMouse" },
            { VK_XBUTTON1, "Mouse4" },
            { VK_XBUTTON2, "Mouse5" },
            { VK_BACK, "Backspace" },
            { VK_TAB, "Tab" },
            { VK_RETURN, "Enter" },
            { VK_PAUSE, "Pause" },
            { VK_CAPITAL, "CapsLock" },
            { VK_SPACE, "Space" },
            { VK_PRIOR, "PageUp" },
            { VK_NEXT, "PageDown" },
            { VK_END, "End" },
            { VK_HOME, "Home" },
            { VK_LEFT, "Left" },
            { VK_UP, "Up" },
            { VK_RIGHT, "Right" },
            { VK_DOWN, "Down" },
            { VK_SNAPSHOT, "PrintScreen" },
            { VK_INSERT, "Insert" },
            { VK_DELETE, "Delete" },
            { VK_NUMPAD0, "Num0" },
            { VK_NUMPAD1, "Num1" },
            { VK_NUMPAD2, "Num2" },
            { VK_NUMPAD3, "Num3" },
            { VK_NUMPAD4, "Num4" },
            { VK_NUMPAD5, "Num5" },
            { VK_NUMPAD6, "Num6" },
            { VK_NUMPAD7, "Num7" },
            { VK_NUMPAD8, "Num8" },
            { VK_NUMPAD9, "Num9" },
            { VK_MULTIPLY, "NumMultiply" },
            { VK_ADD, "NumAdd" },
            { VK_SUBTRACT, "NumSubtract" },
            { VK_DECIMAL, "NumDecimal" },
            { VK_DIVIDE, "NumDivide" },
            { VK_NUMLOCK, "NumLock" },
            { VK_SCROLL, "ScrollLock" },
        };

        return s_Keys;
    }

    std::string Lower(std::string p_Text) {
        std::ranges::transform(p_Text, p_Text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return p_Text;
    }

    std::string Trim(const std::string& p_Text) {
        const auto s_Begin = p_Text.find_first_not_of(" \t");

        if (s_Begin == std::string::npos) {
            return {};
        }

        const auto s_End = p_Text.find_last_not_of(" \t");
        return p_Text.substr(s_Begin, s_End - s_Begin + 1);
    }

    std::string KeyToken(uint8_t p_Vk) {
        if ((p_Vk >= 'A' && p_Vk <= 'Z') || (p_Vk >= '0' && p_Vk <= '9')) {
            return std::string(1, static_cast<char>(p_Vk));
        }

        if (p_Vk >= VK_F1 && p_Vk <= VK_F24) {
            return std::format("F{}", p_Vk - VK_F1 + 1);
        }

        const auto& s_Keys = NamedKeys();

        if (const auto s_It = s_Keys.find(p_Vk); s_It != s_Keys.end()) {
            return s_It->second;
        }

        return std::format("0x{:02X}", p_Vk);
    }

    uint8_t ParseKeyToken(const std::string& p_Token) {
        if (p_Token.size() == 1) {
            const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(p_Token[0])));

            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                return static_cast<uint8_t>(c);
            }
        }

        const std::string s_Lower = Lower(p_Token);

        if (s_Lower.size() >= 2 && s_Lower[0] == 'f' && std::isdigit(static_cast<unsigned char>(s_Lower[1]))) {
            try {
                const int s_Number = std::stoi(s_Lower.substr(1));

                if (s_Number >= 1 && s_Number <= 24) {
                    return static_cast<uint8_t>(VK_F1 + s_Number - 1);
                }
            }
            catch (const std::exception&) {}
        }

        if (s_Lower.starts_with("0x")) {
            try {
                const unsigned long s_Value = std::stoul(s_Lower, nullptr, 16);

                if (s_Value > 0 && s_Value < 256) {
                    return static_cast<uint8_t>(s_Value);
                }
            }
            catch (const std::exception&) {}
        }

        for (const auto& [s_Vk, s_Name] : NamedKeys()) {
            if (Lower(s_Name) == s_Lower) {
                return s_Vk;
            }
        }

        return 0;
    }

    // Key name as Windows prints it for this keyboard layout.
    std::string LocalKeyName(uint8_t p_Vk) {
        UINT s_ScanCode = MapVirtualKeyW(p_Vk, MAPVK_VK_TO_VSC);

        if (s_ScanCode == 0) {
            return {};
        }

        LONG s_Param = static_cast<LONG>(s_ScanCode) << 16;

        switch (p_Vk) {
            case VK_INSERT:
            case VK_DELETE:
            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_LEFT:
            case VK_RIGHT:
            case VK_UP:
            case VK_DOWN:
            case VK_DIVIDE:
                s_Param |= 1 << 24;
                break;
            default:
                break;
        }

        wchar_t s_Name[64] {};

        if (GetKeyNameTextW(s_Param, s_Name, 64) <= 0) {
            return {};
        }

        char s_Utf8[256] {};
        WideCharToMultiByte(CP_UTF8, 0, s_Name, -1, s_Utf8, sizeof(s_Utf8), nullptr, nullptr);
        return s_Utf8;
    }
}

HotkeyCombo HotkeyManager::Parse(const std::string& p_Text) {
    HotkeyCombo s_Combo;
    std::string s_Rest = Trim(p_Text);

    if (s_Rest.empty() || Lower(s_Rest) == "none") {
        return s_Combo;
    }

    size_t s_Start = 0;

    while (s_Start <= s_Rest.size()) {
        const size_t s_Plus = s_Rest.find('+', s_Start);
        const std::string s_Token = Trim(s_Rest.substr(s_Start, s_Plus == std::string::npos ? std::string::npos : s_Plus - s_Start));
        const std::string s_Lower = Lower(s_Token);

        if (s_Lower == "ctrl" || s_Lower == "control") {
            s_Combo.Ctrl = true;
        }
        else if (s_Lower == "alt") {
            s_Combo.Alt = true;
        }
        else if (s_Lower == "shift") {
            s_Combo.Shift = true;
        }
        else if (!s_Token.empty()) {
            s_Combo.Vk = ParseKeyToken(s_Token);
        }

        if (s_Plus == std::string::npos) {
            break;
        }

        s_Start = s_Plus + 1;
    }

    if (s_Combo.Vk == 0) {
        return {};
    }

    return s_Combo;
}

std::string HotkeyManager::Serialize(const HotkeyCombo& p_Combo) {
    if (p_Combo.Empty()) {
        return "None";
    }

    std::string s_Text;

    if (p_Combo.Ctrl) s_Text += "Ctrl+";
    if (p_Combo.Alt) s_Text += "Alt+";
    if (p_Combo.Shift) s_Text += "Shift+";

    return s_Text + KeyToken(p_Combo.Vk);
}

std::string HotkeyManager::Display(const HotkeyCombo& p_Combo) {
    if (p_Combo.Empty()) {
        return "None";
    }

    std::string s_Text;

    if (p_Combo.Ctrl) s_Text += "Ctrl+";
    if (p_Combo.Alt) s_Text += "Alt+";
    if (p_Combo.Shift) s_Text += "Shift+";

    std::string s_Key = KeyToken(p_Combo.Vk);

    // A key without a portable name ("0xDE") is displayed with the layout's name.
    if (s_Key.starts_with("0x")) {
        if (const std::string s_Local = LocalKeyName(p_Combo.Vk); !s_Local.empty()) {
            s_Key = s_Local;
        }
    }

    return s_Text + s_Key;
}

void HotkeyManager::NoteMod(IPluginInterface* p_Plugin, const std::string& p_Name) {
    std::scoped_lock s_Lock(m_Mutex);
    m_ModNames[p_Plugin] = p_Name;
}

void HotkeyManager::RemovePlugin(IPluginInterface* p_Plugin) {
    std::scoped_lock s_Lock(m_Mutex);

    std::erase_if(m_Entries, [&](const Entry& p_Entry) { return p_Entry.Plugin == p_Plugin; });
    m_ModNames.erase(p_Plugin);

    if (m_CaptureTarget == CaptureTarget::Entry) {
        m_CaptureTarget = CaptureTarget::None;
    }
}

void HotkeyManager::Register(IPluginInterface* p_Plugin, const char* p_Id, const char* p_Label, const char* p_Default) {
    if (!p_Plugin || !p_Id || !*p_Id) {
        return;
    }

    const HotkeyCombo s_Default = Parse(p_Default ? p_Default : "");

    // Read outside the lock: settings take the mod loader's lock.
    const ZString s_Stored = ModSDK::GetInstance()->GetPluginSetting(
        p_Plugin, "hotkeys", std::string(p_Id), std::string(Serialize(s_Default))
    );

    const HotkeyCombo s_Combo = Parse(std::string(s_Stored.c_str(), s_Stored.size()));

    std::scoped_lock s_Lock(m_Mutex);

    std::string s_Mod = "?";

    if (const auto s_It = m_ModNames.find(p_Plugin); s_It != m_ModNames.end()) {
        s_Mod = s_It->second;
    }

    if (!s_Mod.empty()) {
        s_Mod[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s_Mod[0])));
    }

    for (auto& s_Entry : m_Entries) {
        if (s_Entry.Plugin == p_Plugin && s_Entry.Id == p_Id) {
            s_Entry.Label = p_Label ? p_Label : p_Id;
            s_Entry.Default = s_Default;
            s_Entry.Combo = s_Combo;
            return;
        }
    }

    m_Entries.push_back(
        Entry {
            .Plugin = p_Plugin,
            .Mod = s_Mod,
            .Id = p_Id,
            .Label = p_Label ? p_Label : p_Id,
            .Default = s_Default,
            .Combo = s_Combo,
        }
    );

    std::ranges::stable_sort(m_Entries, [](const Entry& a, const Entry& b) { return a.Mod < b.Mod; });
}

bool HotkeyManager::KeyDown(int p_Vk) {
    return (GetAsyncKeyState(p_Vk) & 0x8000) != 0;
}

bool HotkeyManager::IsModifierVk(int p_Vk) {
    switch (p_Vk) {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_LMENU:
        case VK_RMENU:
        case VK_LWIN:
        case VK_RWIN:
            return true;
        default:
            return false;
    }
}

bool HotkeyManager::ComboDown(const HotkeyCombo& p_Combo) {
    if (p_Combo.Empty() || !KeyDown(p_Combo.Vk)) {
        return false;
    }

    // Modifiers must match exactly, so "I" and "Alt+I" can be bound
    // to two different things.
    return KeyDown(VK_CONTROL) == p_Combo.Ctrl && KeyDown(VK_MENU) == p_Combo.Alt &&
        KeyDown(VK_SHIFT) == p_Combo.Shift;
}

bool HotkeyManager::InputAllowed() {
    const HWND s_Foreground = GetForegroundWindow();

    if (!s_Foreground) {
        return false;
    }

    DWORD s_ProcessId = 0;
    GetWindowThreadProcessId(s_Foreground, &s_ProcessId);

    if (s_ProcessId != GetCurrentProcessId()) {
        return false;
    }

    const auto s_Renderer = ModSDK::GetInstance()->GetImguiRenderer();
    return !s_Renderer || !s_Renderer->HasFocus();
}

bool HotkeyManager::IsPressed(IPluginInterface* p_Plugin, const char* p_Id) {
    if (!p_Id) {
        return false;
    }

    std::scoped_lock s_Lock(m_Mutex);

    for (auto& s_Entry : m_Entries) {
        if (s_Entry.Plugin != p_Plugin || s_Entry.Id != p_Id) {
            continue;
        }

        const bool s_Down = m_CaptureTarget == CaptureTarget::None && InputAllowed() && ComboDown(s_Entry.Combo);
        const bool s_Pressed = s_Down && !s_Entry.WasDown;
        s_Entry.WasDown = s_Down;
        return s_Pressed;
    }

    return false;
}

std::string HotkeyManager::Text(IPluginInterface* p_Plugin, const char* p_Id) {
    if (!p_Id) {
        return "None";
    }

    std::scoped_lock s_Lock(m_Mutex);

    for (const auto& s_Entry : m_Entries) {
        if (s_Entry.Plugin == p_Plugin && s_Entry.Id == p_Id) {
            return Display(s_Entry.Combo);
        }
    }

    return "None";
}

std::string HotkeyManager::ScanCodeName(uint8_t p_ScanCode) {
    wchar_t s_Name[64] {};

    if (GetKeyNameTextW(static_cast<LONG>(p_ScanCode) << 16, s_Name, 64) <= 0) {
        return std::format("Scancode 0x{:02X}", p_ScanCode);
    }

    char s_Utf8[256] {};
    WideCharToMultiByte(CP_UTF8, 0, s_Name, -1, s_Utf8, sizeof(s_Utf8), nullptr, nullptr);
    return s_Utf8;
}

void HotkeyManager::BeginCapture(CaptureTarget p_Target, size_t p_Index) {
    m_CaptureTarget = p_Target;
    m_CaptureIndex = p_Index;

    // Anything already held down (the click on the button) is ignored.
    for (int s_Vk = 1; s_Vk < 256; ++s_Vk) {
        m_CaptureBaseline[s_Vk] = KeyDown(s_Vk);
    }
}

void HotkeyManager::UpdateCapture() {
    if (m_CaptureTarget == CaptureTarget::None) {
        return;
    }

    int s_NewKey = 0;

    for (int s_Vk = 1; s_Vk < 256; ++s_Vk) {
        const bool s_Down = KeyDown(s_Vk);

        if (s_Down && !m_CaptureBaseline[s_Vk] && s_NewKey == 0 && !IsModifierVk(s_Vk) &&
            s_Vk != VK_LBUTTON && s_Vk != VK_RBUTTON) {
            s_NewKey = s_Vk;
        }

        m_CaptureBaseline[s_Vk] = s_Down;
    }

    if (s_NewKey == 0) {
        return;
    }

    const CaptureTarget s_Target = m_CaptureTarget;
    m_CaptureTarget = CaptureTarget::None;
    m_CaptureCooldown = 15;

    if (s_NewKey == VK_ESCAPE) {
        return;
    }

    if (s_Target == CaptureTarget::Entry) {
        HotkeyCombo s_Combo {
            .Ctrl = KeyDown(VK_CONTROL),
            .Alt = KeyDown(VK_MENU),
            .Shift = KeyDown(VK_SHIFT),
            .Vk = static_cast<uint8_t>(s_NewKey),
        };

        SetEntryCombo(m_CaptureIndex, s_Combo);
        return;
    }

    // SDK keys: a single key, identified by its scan code.
    const UINT s_ScanCode = MapVirtualKeyW(s_NewKey, MAPVK_VK_TO_VSC);

    if (s_ScanCode == 0 || s_ScanCode > 0xFF) {
        Logger::Warn("[Hotkeys] This key cannot be used to toggle the SDK menu.");
        return;
    }

    if (s_Target == CaptureTarget::ConsoleKey) {
        ModSDK::GetInstance()->SetConsoleScanCode(static_cast<uint8_t>(s_ScanCode));
    }
    else {
        ModSDK::GetInstance()->SetUIToggleScanCode(static_cast<uint8_t>(s_ScanCode));
    }
}

void HotkeyManager::SetEntryCombo(size_t p_Index, const HotkeyCombo& p_Combo) {
    if (p_Index >= m_Entries.size()) {
        return;
    }

    auto& s_Entry = m_Entries[p_Index];
    s_Entry.Combo = p_Combo;
    s_Entry.WasDown = true; // do not trigger with the key that was just chosen

    Logger::Info("[Hotkeys] {} / {}: {}", s_Entry.Mod, s_Entry.Id, Serialize(p_Combo));
}

void HotkeyManager::SaveEntry(const Entry& p_Entry) {
    ModSDK::GetInstance()->SetPluginSetting(
        p_Entry.Plugin, "hotkeys", std::string(p_Entry.Id), std::string(Serialize(p_Entry.Combo))
    );
}

void HotkeyManager::Draw(bool p_HasFocus) {
    if (!p_HasFocus || !m_Open) {
        std::scoped_lock s_Lock(m_Mutex);
        m_CaptureTarget = CaptureTarget::None;
        return;
    }

    std::vector<Entry> s_ToSave;

    {
        std::scoped_lock s_Lock(m_Mutex);

        std::vector<HotkeyCombo> s_Before;
        s_Before.reserve(m_Entries.size());

        for (const auto& s_Entry : m_Entries) {
            s_Before.push_back(s_Entry.Combo);
        }

        if (m_CaptureCooldown > 0) {
            --m_CaptureCooldown;
        }

        UpdateCapture();

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        // Minimum height from the content: a size saved in a previous session would
        // otherwise hide shortcuts added since, with no visible scrollbar.
        {
            std::unordered_set<std::string> s_Mods;

            for (const auto& s_Entry : m_Entries) {
                s_Mods.insert(s_Entry.Mod);
            }

            const float s_Row = ImGui::GetFrameHeightWithSpacing();
            const float s_MinHeight = s_Row * (5.f + 2.f + 1.6f * static_cast<float>(1 + s_Mods.size()) +
                static_cast<float>(m_Entries.size()));

            ImGui::SetNextWindowSizeConstraints(
                ImVec2(680.f, std::max(420.f, s_MinHeight)), ImVec2(FLT_MAX, FLT_MAX)
            );
        }
        const bool s_Expanded = ImGui::Begin(ICON_MD_KEYBOARD " Keyboard shortcuts", &m_Open);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_Expanded) {
            ImGui::TextWrapped(
                "Click a shortcut, then press the new key or combination (Ctrl, Alt, Shift + a key). "
                "Esc cancels. Shortcuts only work while the SDK menu is closed."
            );

            ImGui::Spacing();
            ImGui::SeparatorText("SDK");

            const auto s_Sdk = ModSDK::GetInstance();

            const auto s_SdkRow = [&](const char* p_Label, CaptureTarget p_Target, uint8_t p_Current, uint8_t p_Default,
                                     const char* p_Id) {
                ImGui::PushID(p_Id);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(p_Label);
                ImGui::TableNextColumn();

                const bool s_Capturing = m_CaptureTarget == p_Target;
                const std::string s_Text = s_Capturing ? "Press a key..." : ScanCodeName(p_Current);

                if (ImGui::Button(s_Text.c_str(), ImVec2(-FLT_MIN, 0.f)) && !s_Capturing && m_CaptureCooldown == 0) {
                    BeginCapture(p_Target, 0);
                }

                ImGui::TableNextColumn();

                ImGui::BeginDisabled(p_Current == p_Default);

                if (ImGui::Button("Reset")) {
                    if (p_Target == CaptureTarget::ConsoleKey) {
                        s_Sdk->SetConsoleScanCode(p_Default);
                    }
                    else {
                        s_Sdk->SetUIToggleScanCode(p_Default);
                    }
                }

                ImGui::EndDisabled();
                ImGui::TableNextColumn();
                ImGui::PopID();
            };

            if (ImGui::BeginTable("##SdkHotkeys", 4, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 3.f);
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 2.f);
                ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed);

                s_SdkRow("Open / close the SDK menu", CaptureTarget::ConsoleKey, s_Sdk->GetConsoleScanCode(), 0x29, "console");
                s_SdkRow("Hide / show the SDK interface", CaptureTarget::UIToggleKey, s_Sdk->GetUIToggleScanCode(), 0x57, "ui");

                ImGui::EndTable();
            }

            std::string s_CurrentMod;
            bool s_TableOpen = false;

            for (size_t i = 0; i < m_Entries.size(); ++i) {
                auto& s_Entry = m_Entries[i];

                if (s_Entry.Mod != s_CurrentMod || !s_TableOpen) {
                    if (s_TableOpen) {
                        ImGui::EndTable();
                        s_TableOpen = false;
                    }

                    s_CurrentMod = s_Entry.Mod;
                    ImGui::Spacing();
                    ImGui::SeparatorText(s_CurrentMod.c_str());

                    s_TableOpen = ImGui::BeginTable(
                        std::format("##Hotkeys{}", s_CurrentMod).c_str(), 4, ImGuiTableFlags_SizingStretchProp
                    );

                    if (s_TableOpen) {
                        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 3.f);
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 2.f);
                        ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed);
                    }
                }

                if (!s_TableOpen) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(s_Entry.Label.c_str());

                // Flag a combination already used elsewhere (both would trigger).
                std::string s_Conflict;

                if (!s_Entry.Combo.Empty()) {
                    for (size_t j = 0; j < m_Entries.size(); ++j) {
                        if (j != i && m_Entries[j].Combo == s_Entry.Combo) {
                            s_Conflict = std::format("Also used by {}: {}", m_Entries[j].Mod, m_Entries[j].Label);
                            break;
                        }
                    }
                }

                if (!s_Conflict.empty()) {
                    ImGui::TextColored(ImVec4(1.f, 0.45f, 0.35f, 1.f), "%s", s_Conflict.c_str());
                }

                ImGui::TableNextColumn();

                const bool s_Capturing = m_CaptureTarget == CaptureTarget::Entry && m_CaptureIndex == i;
                const std::string s_Text = s_Capturing ? "Press keys..." : Display(s_Entry.Combo);

                if (ImGui::Button(s_Text.c_str(), ImVec2(-FLT_MIN, 0.f)) && !s_Capturing && m_CaptureCooldown == 0) {
                    BeginCapture(CaptureTarget::Entry, i);
                }

                ImGui::TableNextColumn();
                ImGui::BeginDisabled(s_Entry.Combo == s_Entry.Default);

                if (ImGui::Button("Reset")) {
                    SetEntryCombo(i, s_Entry.Default);
                }

                ImGui::EndDisabled();
                ImGui::TableNextColumn();
                ImGui::BeginDisabled(s_Entry.Combo.Empty());

                if (ImGui::Button("Clear")) {
                    SetEntryCombo(i, {});
                }

                ImGui::EndDisabled();
                ImGui::PopID();
            }

            if (s_TableOpen) {
                ImGui::EndTable();
            }

            if (m_Entries.empty()) {
                ImGui::TextDisabled("No mod has declared a shortcut.");
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();

        for (size_t i = 0; i < m_Entries.size() && i < s_Before.size(); ++i) {
            if (!(m_Entries[i].Combo == s_Before[i])) {
                s_ToSave.push_back(m_Entries[i]);
            }
        }
    }

    // Written outside the lock: settings take the mod loader's lock, which mod
    // unloading holds while calling RemovePlugin.
    for (const auto& s_Entry : s_ToSave) {
        SaveEntry(s_Entry);
    }
}
