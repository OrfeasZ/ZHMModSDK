#include <Editor.h>
#include <winhttp.h>
#include <simdjson.h>
#include <thread>
#include <future>
#include <algorithm>
#include <cctype>

#include "Logging.h"
#include "IconsMaterialDesign.h"

#pragma comment(lib, "winhttp.lib")

struct LibraryItem {
    std::string Name;
    std::optional<ZRuntimeResourceID> ResId;
    std::vector<std::shared_ptr<LibraryItem>> Children;
};

// Helper function to download content from URL
std::string DownloadFromUrl(const std::string& url) {
    std::string result;

    // Parse URL
    std::wstring wideUrl(url.begin(), url.end());
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {};
    wchar_t urlPath[1024] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp)) {
        return result;
    }

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"ZHMModSDK/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0
    );
    if (!hSession) {
        return result;
    }

    // Connect to server
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Create request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", urlPath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags
    );
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Send request
    if (WinHttpSendRequest(
            hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0
        ) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;

            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                result.append(buffer.data(), bytesRead);
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

// Helper function to parse JSON and build library items
std::vector<std::shared_ptr<LibraryItem>> ParseHashesJson(const std::string& p_HashesJson) {
    // ResId -> Path mapping
    // Example: 006D2D1E3D8C6598 -> [assembly:/_pro/_licensed/quixel/geometry/foliage/quixel_nasturtium_rijwe_a.wl2?/quixel_nasturtium_rijwe_a.prim].pc_entitytype
    std::vector<std::tuple<ZRuntimeResourceID, std::string>> s_Files;

    try {
        simdjson::ondemand::parser s_Parser;
        simdjson::padded_string s_Json(p_HashesJson);
        simdjson::ondemand::document s_Doc = s_Parser.iterate(s_Json);

        for (auto s_File : s_Doc.get_array()) {
            std::string_view s_HashStr = s_File["hash"];
            std::string_view s_PathStr = s_File["path"];
            int64_t s_GameFlags = s_File["gameFlags"];

            // If bit 4 is not set, it means this file is not in HM3, so we skip it.
            if ((s_GameFlags & (1 << 4)) == 0) {
                continue;
            }

            s_Files.emplace_back(ZRuntimeResourceID::FromString(std::string(s_HashStr)), s_PathStr);
        }
    }
    catch (const std::exception& e) {
        // Log error but don't crash
        Logger::Error("Failed to parse hashes JSON: {}", e.what());
    }

    // Create a tree using BFS.
    std::map<std::string, std::shared_ptr<LibraryItem>> s_PathToNode;
    std::vector<std::shared_ptr<LibraryItem>> s_RootItems;

    for (const auto& [s_ResId, s_Path] : s_Files) {
        std::string s_CleanPath = s_Path;

        // Remove [assembly: prefix and ].pc_entitytype suffix
        if (s_CleanPath.starts_with("[assembly:")) {
            s_CleanPath = s_CleanPath.substr(std::string("[assembly:").length());
        }

        if (s_CleanPath.ends_with("].pc_entitytype")) {
            s_CleanPath = s_CleanPath.substr(0, s_CleanPath.length() - std::string("].pc_entitytype").length());
        }

        // If it doesn't end in .entitytemplate, skip it.
        if (!s_CleanPath.ends_with(".entitytemplate")) {
            continue;
        }

        // Split path into segments
        std::vector<std::string> s_Segments;
        std::string s_CurrentSegment;

        for (char c : s_CleanPath) {
            if (c == '/') {
                if (!s_CurrentSegment.empty()) {
                    s_Segments.push_back(s_CurrentSegment);
                    s_CurrentSegment.clear();
                }
            }
            else {
                s_CurrentSegment += c;
            }
        }

        if (!s_CurrentSegment.empty()) {
            s_Segments.push_back(s_CurrentSegment);
        }

        // Build tree path by path
        std::shared_ptr<LibraryItem> s_CurrentParent = nullptr;
        std::string s_CurrentPath;

        for (size_t i = 0; i < s_Segments.size(); ++i) {
            if (!s_CurrentPath.empty()) {
                s_CurrentPath += "/";
            }
            s_CurrentPath += s_Segments[i];

            // Check if this path node already exists
            auto s_It = s_PathToNode.find(s_CurrentPath);
            std::shared_ptr<LibraryItem> s_CurrentNode;

            if (s_It != s_PathToNode.end()) {
                s_CurrentNode = s_It->second;
            }
            else {
                // Create new node
                s_CurrentNode = std::make_shared<LibraryItem>();
                s_CurrentNode->Name = s_Segments[i];

                // If this is the last segment, assign the resource ID
                if (i == s_Segments.size() - 1) {
                    s_CurrentNode->ResId = s_ResId;
                }

                s_PathToNode[s_CurrentPath] = s_CurrentNode;

                // Add to parent or root
                if (s_CurrentParent) {
                    s_CurrentParent->Children.push_back(s_CurrentNode);

                    // Sort by name.
                    std::ranges::sort(
                        s_CurrentParent->Children,
                        [](const std::shared_ptr<LibraryItem>& a, const std::shared_ptr<LibraryItem>& b) {
                            return a->Name < b->Name;
                        }
                    );
                }
                else {
                    s_RootItems.push_back(s_CurrentNode);
                }
            }

            s_CurrentParent = s_CurrentNode;
        }
    }

    return s_RootItems;
}

void Editor::DrawLibrary() {
    static std::vector<std::shared_ptr<LibraryItem>> s_LibraryItems;
    static std::future<std::vector<std::shared_ptr<LibraryItem>>> s_DownloadFuture;
    static std::shared_ptr<LibraryItem> s_SelectedItem;
    static std::vector<std::shared_ptr<LibraryItem>> s_CurrentPath;
    static std::string s_SearchFilter;
    static bool s_GridView = true;

    static bool s_DownloadStarted = false;
    static bool s_DownloadCompleted = false;

    if (s_LibraryItems.empty() && !s_DownloadStarted) {
        // Start download in background thread
        const std::string s_HashesUrl =
                "https://raw.githubusercontent.com/glacier-modding/Hitman-Hashes/refs/heads/main/paths/TEMP.json";

        s_DownloadFuture = std::async(
            std::launch::async, [s_HashesUrl]() {
                std::string jsonContent = DownloadFromUrl(s_HashesUrl);

                if (!jsonContent.empty()) {
                    return ParseHashesJson(jsonContent);
                }

                return std::vector<std::shared_ptr<LibraryItem>>();
            }
        );

        s_DownloadStarted = true;
    }

    // Check if download is complete
    if (s_DownloadStarted && !s_DownloadCompleted) {
        if (s_DownloadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            s_LibraryItems = s_DownloadFuture.get();
            s_DownloadCompleted = true;
            Logger::Debug("Hashlist download complete! Loaded {} items.", s_LibraryItems.size());
        }
    }

    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    ImGui::SetNextWindowPos({615 + 10, ImGui::GetIO().DisplaySize.y - 300}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x - (615 + 10 + 10 + 500), 300}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ICON_MD_LIBRARY_BOOKS " Library")) {
        if (!s_DownloadCompleted) {
            ImGui::Text("Loading library...");
            ImGui::End();
            return;
        }

        if (s_LibraryItems.empty()) {
            ImGui::Text("Failed to load library data.");
            ImGui::End();
            return;
        }

        // Toolbar
        ImGui::BeginChild("LibraryToolbar", ImVec2(0, 30), false, ImGuiWindowFlags_NoScrollbar);

        // Navigation buttons
        if (ImGui::Button(ICON_MD_HOME)) {
            s_CurrentPath.clear();
        }
        ImGui::SameLine();

        if (ImGui::Button(ICON_MD_ARROW_BACK) && !s_CurrentPath.empty()) {
            s_CurrentPath.pop_back();
        }
        ImGui::SameLine();

        // View toggle
        if (ImGui::Button(s_GridView ? ICON_MD_LIST : ICON_MD_GRID_VIEW)) {
            s_GridView = !s_GridView;
        }
        ImGui::SameLine();

        // Search filter
        ImGui::SetNextItemWidth(200);
        char s_SearchBuffer[256];
        strncpy_s(s_SearchBuffer, s_SearchFilter.c_str(), sizeof(s_SearchBuffer) - 1);
        if (ImGui::InputText(ICON_MD_SEARCH " Filter", s_SearchBuffer, sizeof(s_SearchBuffer))) {
            s_SearchFilter = s_SearchBuffer;
        }

        ImGui::EndChild();

        // Breadcrumb navigation
        ImGui::Separator();
        ImGui::Text("Path: ");
        ImGui::SameLine();
        if (ImGui::SmallButton("Root")) {
            s_CurrentPath.clear();
        }

        for (size_t i = 0; i < s_CurrentPath.size(); ++i) {
            ImGui::SameLine();
            ImGui::Text(" / ");
            ImGui::SameLine();
            if (ImGui::SmallButton(s_CurrentPath[i]->Name.c_str())) {
                s_CurrentPath.resize(i + 1);
                break;
            }
        }

        ImGui::Separator();

        // Get current items to display
        std::vector<std::shared_ptr<LibraryItem>> s_CurrentItems;
        if (s_CurrentPath.empty()) {
            s_CurrentItems = s_LibraryItems;
        }
        else {
            s_CurrentItems = s_CurrentPath.back()->Children;
        }

        // Apply search filter
        std::vector<std::shared_ptr<LibraryItem>> s_FilteredItems;
        if (s_SearchFilter.empty()) {
            s_FilteredItems = s_CurrentItems;
        }
        else {
            for (const auto& item : s_CurrentItems) {
                std::string lowerName = item->Name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::string lowerFilter = s_SearchFilter;
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

                if (lowerName.find(lowerFilter) != std::string::npos) {
                    s_FilteredItems.push_back(item);
                }
            }
        }

        // Display items
        ImGui::BeginChild("LibraryContent");

        if (s_GridView) {
            // Grid view
            const float s_ItemSize = 80.0f;
            const float s_ItemSpacing = 10.0f;
            const float s_WindowWidth = ImGui::GetContentRegionAvail().x;
            const int s_ColumnsCount = max(1, (int)((s_WindowWidth + s_ItemSpacing) / (s_ItemSize + s_ItemSpacing)));

            ImGui::Columns(s_ColumnsCount, nullptr, false);

            for (const auto& item : s_FilteredItems) {
                const bool s_IsFolder = !item->Children.empty();
                const bool s_IsSelected = (s_SelectedItem == item);

                if (s_IsSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                }

                // Item button
                if (ImGui::Button(
                    fmt::format(
                        "{}##item_{}", s_IsFolder ? ICON_MD_FOLDER : ICON_MD_FILE_COPY, fmt::ptr(item.get())
                    ).c_str(), ImVec2(s_ItemSize, s_ItemSize)
                )) {
                    if (s_IsFolder) {
                        s_CurrentPath.push_back(item);
                    }
                    else {
                        s_SelectedItem = item;
                    }
                }

                if (s_IsSelected) {
                    ImGui::PopStyleColor();
                }

                // Item label
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + s_ItemSize);
                ImGui::TextWrapped("%s", item->Name.c_str());
                ImGui::PopTextWrapPos();

                // Tooltip with full path and resource ID
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", item->Name.c_str());
                    if (item->ResId.has_value()) {
                        ImGui::Text("Resource ID: %016llX", item->ResId->GetID());
                    }
                    if (s_IsFolder) {
                        ImGui::Text("Items: %zu", item->Children.size());
                    }
                    ImGui::EndTooltip();
                }

                ImGui::NextColumn();
            }

            ImGui::Columns(1);
        }
        else {
            // List view
            if (ImGui::BeginTable(
                "LibraryTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY
            )) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Resource ID", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                for (const auto& item : s_FilteredItems) {
                    const bool s_IsFolder = !item->Children.empty();
                    const bool s_IsSelected = (s_SelectedItem == item);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    if (s_IsSelected) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt));
                    }

                    // Name column with icon
                    if (ImGui::Selectable(
                        fmt::format("{} {}", s_IsFolder ? ICON_MD_FOLDER : ICON_MD_FILE_COPY, item->Name).c_str(),
                        s_IsSelected, ImGuiSelectableFlags_SpanAllColumns
                    )) {
                        if (s_IsFolder) {
                            s_CurrentPath.push_back(item);
                        }
                        else {
                            s_SelectedItem = item;
                        }
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", s_IsFolder ? "Folder" : "File");

                    ImGui::TableNextColumn();
                    if (item->ResId.has_value()) {
                        ImGui::Text("%016llX", item->ResId->GetID());
                    }
                    else {
                        ImGui::Text("-");
                    }
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();
}