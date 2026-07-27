#pragma once

#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

namespace Util {
    class HttpUtils {
    public:
        // Helper function to download content from URL
        static std::string DownloadFromUrl(const std::string& url) {
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
    };
}