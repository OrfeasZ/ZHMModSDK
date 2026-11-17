#pragma once

#include <string>
#include <functional>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "StringUtils.h"

namespace Util {
    class ImGuiUtils {
    private:
        struct AutocompleteState {
            int32_t m_SelectedIndex = 0;
            int32_t m_ItemCount = 0;
            bool m_SelectionChanged = false;
            std::string m_PreviousInput;
        };

        static AutocompleteState& GetAutocompleteState(ImGuiID p_Id) {
            static std::unordered_map<ImGuiID, AutocompleteState> s_States;
            return s_States[p_Id];
        }

        static int32_t AutocompleteInputCallback(ImGuiInputTextCallbackData* p_Data) {
            auto* s_State = static_cast<AutocompleteState*>(p_Data->UserData);

            if (p_Data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
                return 0;
            }

            const int32_t s_PreviousIndex = s_State->m_SelectedIndex;

            if (p_Data->EventKey == ImGuiKey_DownArrow) {
                s_State->m_SelectedIndex = std::min(
                    s_State->m_SelectedIndex + 1,
                    std::max(0, s_State->m_ItemCount - 1)
                );
            }
            else if (p_Data->EventKey == ImGuiKey_UpArrow) {
                s_State->m_SelectedIndex = std::max(
                    s_State->m_SelectedIndex - 1,
                    0
                );
            }

            s_State->m_SelectionChanged |= s_State->m_SelectedIndex != s_PreviousIndex;

            return 0;
        }

        template <
            typename Collection,
            typename GetOptionIdFn,
            typename GetOptionNameFn,
            typename OnSelectFn,
            typename SetInputFn,
            typename GetOptionValueFn,
            typename FilterFn
        >
        static bool DrawAutocompletePopup(
            const char* p_Id,
            const std::string& p_Input,
            const Collection& p_Collection,
            GetOptionIdFn p_GetOptionIdFn,
            GetOptionNameFn p_GetOptionNameFn,
            OnSelectFn p_OnSelectFn,
            SetInputFn p_SetInputFn,
            GetOptionValueFn p_GetOptionValueFn,
            FilterFn p_FilterFn,
            bool p_IsCaseSensitiveSearch,
            float p_PopupHeight,
            bool p_IsEnterPressed,
            bool p_IsActive,
            bool p_IsActivated,
            AutocompleteState& p_State
        ) {
            if (p_Input != p_State.m_PreviousInput) {
                p_State.m_SelectedIndex = 0;
                p_State.m_SelectionChanged = true;
                p_State.m_PreviousInput = p_Input;
            }

            if (p_IsActivated) {
                p_State.m_SelectedIndex = 0;
                p_State.m_SelectionChanged = true;
                ImGui::OpenPopup(p_Id);
            }

            const ImVec2 s_InputMin = ImGui::GetItemRectMin();
            const ImVec2 s_InputMax = ImGui::GetItemRectMax();

            ImGui::SetNextWindowPos(ImVec2(s_InputMin.x, s_InputMax.y));
            ImGui::SetNextWindowSize(ImVec2(s_InputMax.x - s_InputMin.x, p_PopupHeight));

            const bool s_IsOpen = ImGui::BeginPopup(
                p_Id,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_HorizontalScrollbar |
                ImGuiWindowFlags_NoFocusOnAppearing
            );

            if (!s_IsOpen) {
                return false;
            }

            if (ImGui::IsWindowAppearing()) {
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            }

            // Collect matching items once.
            using ItemType = std::remove_reference_t<
                decltype(*std::begin(p_Collection))
            >;

            std::vector<const ItemType*> s_Matches;

            for (const auto& s_Item : p_Collection) {
                if constexpr (!std::is_same_v<FilterFn, std::nullptr_t>) {
                    if (!p_FilterFn(s_Item)) {
                        continue;
                    }
                }

                const auto& s_OptionName = p_GetOptionNameFn(s_Item);

                if (!Util::StringUtils::FindSubstring(s_OptionName, p_Input, p_IsCaseSensitiveSearch)) {
                    continue;
                }

                s_Matches.push_back(&s_Item);
            }

            p_State.m_ItemCount = static_cast<int32_t>(s_Matches.size());

            if (p_State.m_ItemCount > 0) {
                p_State.m_SelectedIndex = std::clamp(p_State.m_SelectedIndex, 0, p_State.m_ItemCount - 1);
            }
            else {
                p_State.m_SelectedIndex = 0;
            }

            if (p_State.m_SelectionChanged && p_State.m_ItemCount > 0) {
                ImGuiWindow* s_Window = ImGui::GetCurrentWindow();

                const float s_ItemHeight = ImGui::GetTextLineHeightWithSpacing();
                const float s_ItemTop = p_State.m_SelectedIndex * s_ItemHeight;
                const float s_ItemBottom = s_ItemTop + s_ItemHeight;

                const float s_VisibleHeight = s_Window->InnerRect.GetHeight();

                const float s_ScrollY = ImGui::GetScrollY();

                if (s_ItemTop < s_ScrollY) {
                    ImGui::SetScrollY(s_ItemTop);
                }
                else if (s_ItemBottom > s_ScrollY + s_VisibleHeight) {
                    ImGui::SetScrollY(s_ItemBottom - s_VisibleHeight);
                }
            }

            ImGuiListClipper s_Clipper;
            s_Clipper.Begin(p_State.m_ItemCount);

            if (p_State.m_SelectionChanged && p_State.m_ItemCount > 0) {
                s_Clipper.IncludeItemByIndex(p_State.m_SelectedIndex);
            }

            bool s_OptionSelected = false;

            while (s_Clipper.Step()) {
                for (int32_t i = s_Clipper.DisplayStart; i < s_Clipper.DisplayEnd; ++i) {
                    const auto& s_Item = *s_Matches[i];
                    const auto& s_OptionName = p_GetOptionNameFn(s_Item);
                    const auto& s_OptionId = p_GetOptionIdFn(s_Item);

                    PushID(s_OptionId);

                    const bool s_IsHighlighted = i == p_State.m_SelectedIndex;

                    const bool s_IsSelected = ImGui::Selectable(s_OptionName.c_str(), s_IsHighlighted);

                    if (s_IsHighlighted && p_State.m_SelectionChanged) {
                        ImGuiWindow* s_Window = ImGui::GetCurrentWindow();

                        const ImRect s_ItemRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

                        if (s_ItemRect.Min.y < s_Window->ClipRect.Min.y) {
                            ImGui::SetScrollHereY(0.0f);
                        }
                        else if (s_ItemRect.Max.y > s_Window->ClipRect.Max.y) {
                            ImGui::SetScrollHereY(1.0f);
                        }
                    }

                    ImGui::PopID();

                    if (s_IsSelected) {
                        p_SetInputFn(s_OptionName);

                        if constexpr (!std::is_same_v<GetOptionValueFn, std::nullptr_t>) {
                            auto s_OptionValue = p_GetOptionValueFn(s_Item);
                            p_OnSelectFn(s_OptionId, s_OptionName, s_OptionValue);
                        }
                        else {
                            p_OnSelectFn(s_OptionId, s_OptionName, s_Item);
                        }

                        p_State.m_PreviousInput = s_OptionName;
                        s_OptionSelected = true;
                        break;
                    }
                }

                if (s_OptionSelected) {
                    break;
                }
            }

            s_Clipper.End();

            if (p_IsEnterPressed && !s_OptionSelected && p_State.m_ItemCount > 0) {
                const auto& s_Item = *s_Matches[p_State.m_SelectedIndex];
                const auto& s_OptionName = p_GetOptionNameFn(s_Item);
                const auto& s_OptionId = p_GetOptionIdFn(s_Item);

                p_SetInputFn(s_OptionName);

                if constexpr (!std::is_same_v<GetOptionValueFn, std::nullptr_t>) {
                    auto s_OptionValue = p_GetOptionValueFn(s_Item);

                    p_OnSelectFn(s_OptionId, s_OptionName, s_OptionValue);
                }
                else {
                    p_OnSelectFn(s_OptionId, s_OptionName, s_Item);
                }

                p_State.m_PreviousInput = s_OptionName;
                s_OptionSelected = true;
            }

            p_State.m_SelectionChanged = false;

            if (s_OptionSelected || p_IsEnterPressed || (!p_IsActive && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();

            return true;
        }

        template<typename T> static void PushID(const T& p_Id) {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                ImGui::PushID(p_Id.c_str());
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
                ImGui::PushID(p_Id.data(), p_Id.data() + p_Id.size());
            }
            else if constexpr (std::is_integral_v<std::decay_t<T>>) {
                ImGui::PushID(reinterpret_cast<const char*>(&p_Id), reinterpret_cast<const char*>(&p_Id) + sizeof(p_Id));
            }
            else {
                static_assert(false, "Unsupported ID type");
            }
        }

    public:
        /**
         * @brief Renders an ImGui input field with autocomplete functionality.
         *
         * Displays a popup with filtered options from the provided collection as the user types.
         * Items are matched based on the input text, and optionally a user-provided filter callback.
         * When an option is selected, it automatically fills the input field and triggers a callback.
         *
         * @tparam Collection             Iterable collection (e.g., TMap, std::vector) containing items.
         * @tparam GetOptionIdFn          Function or lambda that returns a unique ID for an item.
         * @tparam GetOptionNameFn        Function or lambda that returns the display name (string) for an item.
         * @tparam OnSelectFn             Callback function executed when an item is selected.
         * @tparam GetOptionValueFn       (Optional) Function or lambda returning an additional value
         *                                passed to OnSelectFn. Defaults to nullptr.
         * @tparam FilterFn               (Optional) Function or lambda used to filter out items before
         *                                text matching. If provided, only items for which the filter
         *                                returns true are considered. Defaults to nullptr.
         *
         * @param p_Id                    Unique identifier for the input and popup.
         * @param p_InputBuffer           Character buffer for the input text.
         * @param p_BufferSize            Size of the input buffer.
         * @param p_Collection            Collection of items to display and search through.
         * @param p_GetOptionIdFn         Function used to extract the item's unique ID.
         * @param p_GetOptionNameFn       Function used to extract the display name (string).
         * @param p_OnSelectFn            Callback executed when an option is selected.
         * @param p_GetOptionValueFn      (Optional) Function that retrieves the value passed to the
         *                                selection callback. If nullptr, the raw item is passed.
         * @param p_FilterFn              (Optional) Predicate function used to filter items before
         *                                substring matching. Items returning false are skipped.
         * @param p_IsCaseSensitiveSearch Whether the search should be case-sensitive.
         * @param p_PopupHeight           Height of the popup window (default: 300.f).
         *
         * @return true if the popup is open, false otherwise.
         */
        template <
            typename Collection,
            typename GetOptionIdFn,
            typename GetOptionNameFn,
            typename OnSelectFn,
            typename GetOptionValueFn = std::nullptr_t,
            typename FilterFn = std::nullptr_t
        >
        static bool InputWithAutocomplete(
            const char* p_Id,
            char* p_InputBuffer,
            size_t p_BufferSize,
            const Collection& p_Collection,
            GetOptionIdFn p_GetOptionIdFn,
            GetOptionNameFn p_GetOptionNameFn,
            OnSelectFn p_OnSelectFn,
            GetOptionValueFn p_GetOptionValueFn = nullptr,
            FilterFn p_FilterFn = nullptr,
            bool p_IsCaseSensitiveSearch = false,
            float p_PopupHeight = 300.f
        ) {
            const std::string s_InputId = std::string(p_Id) + "_Input";

            auto& s_State = GetAutocompleteState(ImGui::GetID(s_InputId.c_str()));

            const bool s_IsEnterPressed = ImGui::InputText(
                s_InputId.c_str(),
                p_InputBuffer,
                p_BufferSize,
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_CallbackHistory,
                AutocompleteInputCallback,
                &s_State
            );

            const bool s_IsActive = ImGui::IsItemActive();
            const bool s_IsActivated = ImGui::IsItemActivated();

            return DrawAutocompletePopup(
                p_Id,
                std::string(p_InputBuffer),
                p_Collection,
                p_GetOptionIdFn,
                p_GetOptionNameFn,
                p_OnSelectFn,
                [&](const std::string& p_Value) {
                    strcpy_s(
                        p_InputBuffer,
                        p_BufferSize,
                        p_Value.c_str()
                    );
                },
                p_GetOptionValueFn,
                p_FilterFn,
                p_IsCaseSensitiveSearch,
                p_PopupHeight,
                s_IsEnterPressed,
                s_IsActive,
                s_IsActivated,
                s_State
            );
        }

        /**
         * @brief Renders an ImGui input field with autocomplete functionality.
         *
         * Displays a popup with filtered options from the provided collection as the user types.
         * Items are matched based on the input text, and optionally a user-provided filter callback.
         * When an option is selected, it automatically fills the input field and triggers a callback.
         *
         * @tparam Collection             Iterable collection (e.g., TMap, std::vector) containing items.
         * @tparam GetOptionIdFn          Function or lambda that returns a unique ID for an item.
         * @tparam GetOptionNameFn        Function or lambda that returns the display name (string) for an item.
         * @tparam OnSelectFn             Callback function executed when an item is selected.
         * @tparam GetOptionValueFn       (Optional) Function or lambda returning an additional value
         *                                passed to OnSelectFn. Defaults to nullptr.
         * @tparam FilterFn               (Optional) Function or lambda used to filter out items before
         *                                text matching. If provided, only items for which the filter
         *                                returns true are considered. Defaults to nullptr.
         *
         * @param p_Id                    Unique identifier for the input and popup.
         * @param p_InputBuffer           Character buffer for the input text.
         * @param p_Collection            Collection of items to display and search through.
         * @param p_GetOptionIdFn         Function used to extract the item's unique ID.
         * @param p_GetOptionNameFn       Function used to extract the display name (string).
         * @param p_OnSelectFn            Callback executed when an option is selected.
         * @param p_GetOptionValueFn      (Optional) Function that retrieves the value passed to the
         *                                selection callback. If nullptr, the raw item is passed.
         * @param p_FilterFn              (Optional) Predicate function used to filter items before
         *                                substring matching. Items returning false are skipped.
         * @param p_IsCaseSensitiveSearch Whether the search should be case-sensitive.
         * @param p_PopupHeight           Height of the popup window (default: 300.f).
         *
         * @return true if the popup is open, false otherwise.
         */
        template <
            typename Collection,
            typename GetOptionIdFn,
            typename GetOptionNameFn,
            typename OnSelectFn,
            typename GetOptionValueFn = std::nullptr_t,
            typename FilterFn = std::nullptr_t
        >
        static bool InputWithAutocomplete(
            const char* p_Id,
            std::string& p_InputBuffer,
            const Collection& p_Collection,
            GetOptionIdFn p_GetOptionIdFn,
            GetOptionNameFn p_GetOptionNameFn,
            OnSelectFn p_OnSelectFn,
            GetOptionValueFn p_GetOptionValueFn = nullptr,
            FilterFn p_FilterFn = nullptr,
            bool p_IsCaseSensitiveSearch = false,
            float p_PopupHeight = 300.f
        ) {
            const std::string s_InputId = std::string(p_Id) + "_Input";

            auto& s_State = GetAutocompleteState(ImGui::GetID(s_InputId.c_str()));

            const bool s_IsEnterPressed = ImGui::InputText(
                s_InputId.c_str(),
                &p_InputBuffer,
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_CallbackHistory,
                AutocompleteInputCallback,
                &s_State
            );

            const bool s_IsActive = ImGui::IsItemActive();
            const bool s_IsActivated = ImGui::IsItemActivated();

            return DrawAutocompletePopup(
                p_Id,
                p_InputBuffer,
                p_Collection,
                p_GetOptionIdFn,
                p_GetOptionNameFn,
                p_OnSelectFn,
                [&](const std::string& p_Value) {
                    p_InputBuffer = p_Value;
                },
                p_GetOptionValueFn,
                p_FilterFn,
                p_IsCaseSensitiveSearch,
                p_PopupHeight,
                s_IsEnterPressed,
                s_IsActive,
                s_IsActivated,
                s_State
            );
        }
    };
}