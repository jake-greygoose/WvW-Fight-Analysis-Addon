#define NOMINMAX
#include "parser/file_helpers.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "evtc_parser.h"
#include "thirdparty/miniz_cpp.hpp"
#include <thread>
#include <chrono>
#include <shlobj.h>

#include <Windows.h>
#include <cstring>
#include <cstdint>
#include <fstream>
// Helper function to convert wide strings to UTF-8
std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();

    std::vector<char> buffer(size);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), size, nullptr, nullptr);

    return std::string(buffer.data());
}

// Get UTF-8 representation of a filesystem path
std::string getUtf8Path(const std::filesystem::path& path) {
    return wideToUtf8(path.wstring());
}

// File operation implementations
std::vector<char> extractZipFile(const std::filesystem::path& filePath) {
    try {
        std::string utf8Path = getUtf8Path(filePath);
        LogMessage(ELogLevel_DEBUG, ("Attempting to extract zip file: " + utf8Path).c_str());

        miniz_cpp::zip_file zip;
        if (!zip.load(filePath)) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Failed to open zip archive: " + utf8Path).c_str());
            return {};
        }

        mz_uint numFiles = zip.get_num_files();
        if (numFiles == 0) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                "Zip archive contains no files.");
            return {};
        }

        mz_zip_archive_file_stat fileStat{};
        mz_uint targetIndex = MZ_UINT32_MAX;

        for (mz_uint i = 0; i < numFiles; ++i) {
            if (!zip.file_stat(i, fileStat)) {
                continue;
            }
            if (fileStat.m_is_directory) {
                continue;
            }
            targetIndex = i;
            break;
        }

        if (targetIndex == MZ_UINT32_MAX) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                "Zip archive does not contain a valid file entry.");
            return {};
        }

        if (fileStat.m_uncomp_size > 100 * 1024 * 1024) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Uncompressed data too large: " + std::to_string(static_cast<unsigned long long>(fileStat.m_uncomp_size)) + " bytes").c_str());
            return {};
        }

        std::vector<char> result;
        try {
            result = zip.read(targetIndex);
        }
        catch (const miniz_cpp::zip_exception& ex) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Failed to extract file from zip archive: " + std::string(ex.what())).c_str());
            return {};
        }

        LogMessage(ELogLevel_DEBUG, ("Successfully extracted zip file: " + utf8Path +
            " (" + std::to_string(result.size()) + " bytes)").c_str());

        return result;
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Unexpected error while extracting zip file: " + std::string(e.what())).c_str());
        return std::vector<char>();
    }
}

void waitForFile(const std::filesystem::path& filePath) {
    try {
        std::string utf8Path = getUtf8Path(filePath);
        LogMessage(ELogLevel_DEBUG, ("Starting to wait for file: " + utf8Path).c_str());

        const int MAX_RETRIES = 30; // 15 seconds total max wait time
        const int RETRY_DELAY_MS = 500;
        int retries = 0;
        DWORD previousSize = 0;
        HANDLE hFile = INVALID_HANDLE_VALUE;

        while (retries < MAX_RETRIES) {
            try {
                hFile = CreateFileW(
                    filePath.wstring().c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);

                if (hFile == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    if (error != ERROR_SHARING_VIOLATION) {
                        LogMessage(ELogLevel_DEBUG, ("File not accessible, error code: " + std::to_string(error)).c_str());
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retries++;
                    continue;
                }

                DWORD currentSize = GetFileSize(hFile, nullptr);
                if (currentSize == INVALID_FILE_SIZE) {
                    DWORD error = GetLastError();
                    APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                        ("Failed to get file size, error code: " + std::to_string(error)).c_str());
                    CloseHandle(hFile);
                    retries++;
                    continue;
                }

                if (currentSize == previousSize && currentSize > 0) {
                    LogMessage(ELogLevel_DEBUG, ("File size stabilized at " + std::to_string(currentSize) + " bytes").c_str());
                    CloseHandle(hFile);
                    break;
                }
                LogMessage(ELogLevel_DEBUG, ("File size changed from " + std::to_string(previousSize) +
                    " to " + std::to_string(currentSize) + " bytes").c_str());
                previousSize = currentSize;
                CloseHandle(hFile);
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                retries++;
            }
            catch (const std::exception& e) {
                if (hFile != INVALID_HANDLE_VALUE) {
                    CloseHandle(hFile);
                }
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    ("Error while checking file: " + std::string(e.what())).c_str());
                throw;
            }
        }

        if (retries >= MAX_RETRIES) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Timeout waiting for file to stabilize: " + utf8Path).c_str());
        }
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Fatal error in waitForFile: " + std::string(e.what())).c_str());
        throw;
    }
}

std::filesystem::path getArcPath()
{
    std::filesystem::path filename = APIDefs->Paths.GetAddonDirectory("arcdps\\arcdps.ini");
    std::string utf8Filename = getUtf8Path(filename);

    char buffer[256] = { 0 };

    GetPrivateProfileStringA(
        "session",
        "boss_encounter_path",
        "",
        buffer,
        sizeof(buffer),
        utf8Filename.c_str()
    );

    std::filesystem::path boss_encounter_path = buffer;

    return boss_encounter_path;
}

// File system monitoring implementations
bool isRunningUnderWine()
{
    if (Settings::forceLinuxCompatibilityMode) {
        return true;
    }
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    else
    {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
    }

    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll)
    {
        FARPROC wine_get_version = GetProcAddress(hNtDll, "wine_get_version");
        if (wine_get_version)
        {
            return true;
        }
    }

    return false;
}


std::vector<char> extractZipFile(const std::string& filePath) {
    return extractZipFile(std::filesystem::path(filePath));
}

void waitForFile(const std::string& filePath) {
    waitForFile(std::filesystem::path(filePath));
}

