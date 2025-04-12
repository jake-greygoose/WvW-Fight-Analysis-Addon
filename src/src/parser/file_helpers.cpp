#define NOMINMAX
#include "parser/file_helpers.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "evtc_parser.h"
#include <zip.h>
#include <thread>
#include <chrono>
#include <shlobj.h>
#include <Windows.h>


// File operation implementations
std::vector<char> extractZipFile(const std::string& filePath) {
    try {
        LogMessage(ELogLevel_DEBUG, ("Attempting to extract zip file: " + filePath).c_str());
        int err = 0;
        zip* z = zip_open(filePath.c_str(), 0, &err);
        if (!z) {
            std::string errMsg = "Failed to open zip file. Error code: " + std::to_string(err) + " " + filePath;
            switch (err) {
            case ZIP_ER_NOENT:
                errMsg += " (File does not exist)";
                break;
            case ZIP_ER_NOZIP:
                errMsg += " (Not a zip file)";
                break;
            case ZIP_ER_INVAL:
                errMsg += " (Invalid argument)";
                break;
            case ZIP_ER_MEMORY:
                errMsg += " (Memory allocation failed)";
                break;
            }
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, errMsg.c_str());
            return std::vector<char>();
        }

        zip_stat_t zstat;
        zip_stat_init(&zstat);
        if (zip_stat_index(z, 0, 0, &zstat) != 0) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                "Failed to get zip file statistics");
            zip_close(z);
            return std::vector<char>();
        }

        // Check for reasonable file size to prevent memory issues
        if (zstat.size > 100 * 1024 * 1024) { // 100MB limit
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Zip file too large: " + std::to_string(zstat.size) + " bytes").c_str());
            zip_close(z);
            return std::vector<char>();
        }

        try {
            std::vector<char> buffer(zstat.size);
            zip_file* f = zip_fopen_index(z, 0, 0);
            if (!f) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    "Failed to open file in zip archive");
                zip_close(z);
                return std::vector<char>();
            }

            zip_int64_t bytesRead = zip_fread(f, buffer.data(), zstat.size);
            if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != zstat.size) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    ("Failed to read complete file. Expected: " + std::to_string(zstat.size) +
                        " bytes, Read: " + std::to_string(bytesRead) + " bytes").c_str());
                zip_fclose(f);
                zip_close(z);
                return std::vector<char>();
            }

            zip_fclose(f);
            zip_close(z);

            LogMessage(ELogLevel_DEBUG, ("Successfully extracted zip file: " + filePath +
                " (" + std::to_string(buffer.size()) + " bytes)").c_str());
            return buffer;
        }
        catch (const std::bad_alloc& e) {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Memory allocation failed for zip buffer: " + std::string(e.what())).c_str());
            zip_close(z);
            return std::vector<char>();
        }
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Unexpected error while extracting zip file: " + std::string(e.what())).c_str());
        return std::vector<char>();
    }
}

void waitForFile(const std::string& filePath) {
    try {
        LogMessage(ELogLevel_DEBUG, ("Starting to wait for file: " + filePath).c_str());

        const int MAX_RETRIES = 30; // 15 seconds total max wait time
        const int RETRY_DELAY_MS = 500;
        int retries = 0;
        DWORD previousSize = 0;
        HANDLE hFile = INVALID_HANDLE_VALUE;

        while (retries < MAX_RETRIES) {
            try {
                hFile = CreateFile(
                    filePath.c_str(),
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
                ("Timeout waiting for file to stabilize: " + filePath).c_str());
        }
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Fatal error in waitForFile: " + std::string(e.what())).c_str());
        throw;
    }
}

std::wstring getCanonicalPath(const std::filesystem::path& path)
{
    return std::filesystem::weakly_canonical(path).wstring();
}

std::filesystem::path getArcPath()
{
    std::filesystem::path filename = APIDefs->Paths.GetAddonDirectory("arcdps\\arcdps.ini");

    char buffer[256] = { 0 };

    GetPrivateProfileStringA(
        "session",
        "boss_encounter_path",
        "",
        buffer,
        sizeof(buffer),
        filename.string().c_str()
    );

    std::filesystem::path boss_encounter_path = buffer;

    return boss_encounter_path;
}

bool isValidEVTCFile(const std::filesystem::path& dirPath, const std::filesystem::path& filePath)
{
    std::filesystem::path relativePath;
    try {
        relativePath = std::filesystem::relative(filePath.parent_path(), dirPath);
    }
    catch (...) {
        return false;
    }

    bool dirPathIsWvW = dirPath.filename().string().find("WvW") != std::string::npos;
    bool dirPathIs1 = dirPath.filename().string() == "1";

    std::vector<std::filesystem::path> components;
    if (!dirPath.has_root_path() && !dirPathIsWvW && !dirPathIs1) {
        components.push_back(dirPath.filename());
    }
    for (const auto& part : relativePath) {
        components.push_back(part);
    }

    int wvwIndex = -1;
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i].string().find("WvW") != std::string::npos) {
            wvwIndex = static_cast<int>(i);
            break;
        }
    }

    int dir1Index = -1;
    if (wvwIndex == -1) {
        for (size_t i = 0; i < components.size(); ++i) {
            if (components[i].string() == "1") {
                dir1Index = static_cast<int>(i);
                break;
            }
        }
    }

    if (dirPathIsWvW) {
        return components.size() <= 2;
    }
    if (wvwIndex != -1) {
        return (components.size() - (wvwIndex + 1)) <= 2;
    }

    if (dirPathIs1) {
        return components.size() <= 2;
    }
    if (dir1Index != -1) {
        return (components.size() - (dir1Index + 1)) <= 2;
    }

    return false;
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

void scanForNewFiles(const std::filesystem::path& dirPath, std::unordered_set<std::wstring>& processedFiles)
{
    try
    {
        std::vector<std::filesystem::path> zevtcFiles;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == L".zevtc")
            {
                if (isValidEVTCFile(dirPath, entry.path()))
                {
                    zevtcFiles.push_back(entry.path());
                }
            }
        }

        std::sort(zevtcFiles.begin(), zevtcFiles.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b)
            {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });

        for (const auto& filePath : zevtcFiles)
        {
            auto fileTime = std::filesystem::last_write_time(filePath);

            if (fileTime <= maxProcessedTime)
            {
                break;
            }

            std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

            if (processedFiles.find(absolutePath) != processedFiles.end())
            {
                continue;
            }

            // Use the path version of processEVTCFile
            processEVTCFile(filePath);

            processedFiles.insert(absolutePath);

            if (fileTime > maxProcessedTime)
            {
                maxProcessedTime = fileTime;
            }
        }
    }
    catch (const std::exception& ex)
    {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Exception in scanning directory: " + std::string(ex.what())).c_str());
    }
}

void parseInitialLogs(std::unordered_set<std::wstring>& processedFiles, size_t numLogsToParse)
{
    try
    {
        std::filesystem::path dirPath;
        if (!Settings::LogDirectoryPath.empty())
        {
            dirPath = std::filesystem::path(Settings::LogDirectoryPath);
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME,
                ("Custom log path specified: " + dirPath.string()).c_str());
            if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    ("Specified log directory does not exist: " + dirPath.string()).c_str());
                return;
            }
        }
        else
        {
            char documentsPath[MAX_PATH];
            if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    "Failed to get path to user's Documents folder.");
                return;
            }

            dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
                "arcdps" / "arcdps.cbtlogs";
        }

        std::vector<std::filesystem::path> zevtcFiles;

        // Recursively search for .zevtc files in the directory
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == L".zevtc")
            {
                if (isValidEVTCFile(dirPath, entry.path()))
                {
                    zevtcFiles.push_back(entry.path());
                }
            }
        }

        if (zevtcFiles.empty())
        {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("No valid .zevtc files found in directory: " + dirPath.string()).c_str());
            return;
        }

        // Sort descending order (newest first)
        std::sort(zevtcFiles.begin(), zevtcFiles.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b)
            {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });

        size_t numFilesToParse = std::min<size_t>(zevtcFiles.size(), numLogsToParse);

        for (size_t i = 0; i < numFilesToParse; ++i)
        {
            const std::filesystem::path& filePath = zevtcFiles[i];
            waitForFile(filePath.string());

            std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

            if (processedFiles.find(absolutePath) == processedFiles.end())
            {
                ParsedLog log;
                log.filename = filePath.filename().string();
                log.data = parseEVTCFile(filePath.string());

                // fightId 1 = WvW
                if (log.data.fightId != 1)
                {
                    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                        ("Skipping non-WvW log during initial parsing: " + log.filename).c_str());
                    continue;
                }

                if (log.data.totalIdentifiedPlayers == 0)
                {
                    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                        ("Skipping log with no identified players during initial parsing: " + log.filename).c_str());
                    continue;
                }

                parsedLogs.push_back(log);

                while (parsedLogs.size() > Settings::logHistorySize)
                {
                    parsedLogs.pop_front();
                }
                currentLogIndex = 0;

                processedFiles.insert(absolutePath);

                // In the Wine implementation we will only parse logs with newer time stamps.
                // Not quite the same behaviour but good enough
                auto fileTime = std::filesystem::last_write_time(filePath);
                if (fileTime > maxProcessedTime)
                {
                    maxProcessedTime = fileTime;
                }
            }
            else
            {
                APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                    ("File already processed during initial parsing: " + filePath.string()).c_str());
            }
        }

        initialParsingComplete = true;
    }
    catch (const std::exception& ex)
    {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Exception in initial parsing: " + std::string(ex.what())).c_str());
    }
}

void directoryMonitor(const std::filesystem::path& dirPath, size_t numLogsToParse)
{
    try
    {
        HANDLE hDir = CreateFileW(
            dirPath.wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);

        if (hDir == INVALID_HANDLE_VALUE)
        {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                ("Failed to open directory for monitoring: " + dirPath.string()).c_str());
            return;
        }

        char buffer[4096];
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        parseInitialLogs(processedFiles, numLogsToParse);

        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
            nullptr,
            &overlapped,
            nullptr);

        if (!success)
        {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to start directory monitoring.");
            CloseHandle(overlapped.hEvent);
            CloseHandle(hDir);
            return;
        }

        while (!stopMonitoring)
        {
            DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 500);

            if (stopMonitoring)
            {
                CancelIoEx(hDir, &overlapped);
                break;
            }

            if (waitStatus == WAIT_OBJECT_0)
            {
                DWORD bytesTransferred = 0;
                if (GetOverlappedResult(hDir, &overlapped, &bytesTransferred, FALSE))
                {
                    BYTE* pBuffer = reinterpret_cast<BYTE*>(buffer);
                    FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pBuffer);

                    do
                    {
                        std::wstring filenameW(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
                        std::filesystem::path fullPath = dirPath / filenameW;

                        // Filter for .zevtc files
                        if ((fni->Action == FILE_ACTION_ADDED ||
                            fni->Action == FILE_ACTION_MODIFIED ||
                            fni->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
                            fullPath.extension() == L".zevtc")
                        {
                            if (isValidEVTCFile(dirPath, fullPath))
                            {
                                std::wstring fullPathStr = std::filesystem::absolute(fullPath).wstring();

                                {
                                    std::lock_guard<std::mutex> lock(processedFilesMutex);

                                    if (processedFiles.find(fullPathStr) == processedFiles.end())
                                    {
                                        processEVTCFile(fullPath);

                                        processedFiles.insert(fullPathStr);
                                    }
                                }
                            }
                        }

                        if (fni->NextEntryOffset == 0)
                            break;

                        fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
                    } while (true);

                    ResetEvent(overlapped.hEvent);

                    success = ReadDirectoryChangesW(
                        hDir,
                        buffer,
                        sizeof(buffer),
                        TRUE,
                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
                        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                        nullptr,
                        &overlapped,
                        nullptr);

                    if (!success)
                    {
                        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                            "Failed to reissue directory monitoring.");
                        break;
                    }
                }
                else
                {
                    DWORD errorCode = GetLastError();
                    APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                        ("GetOverlappedResult failed with error code: " + std::to_string(errorCode)).c_str());
                    break;
                }
            }
            else if (waitStatus == WAIT_TIMEOUT)
            {
                continue;
            }
            else
            {
                DWORD errorCode = GetLastError();
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    ("WaitForSingleObject failed with error code: " + std::to_string(errorCode)).c_str());
                break;
            }
        }

        CloseHandle(overlapped.hEvent);
        CloseHandle(hDir);
    }
    catch (const std::exception& ex)
    {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
    }
}

void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds)
{
    try
    {
        std::filesystem::path dirPath;

        if (firstInstall) {
            std::filesystem::path arcPath = getArcPath();
            Settings::LogDirectoryPath = arcPath.string();

            strncpy(Settings::LogDirectoryPathC, Settings::LogDirectoryPath.c_str(), sizeof(Settings::LogDirectoryPathC) - 1);
            Settings::LogDirectoryPathC[sizeof(Settings::LogDirectoryPathC) - 1] = '\0';

            Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
            Settings::Save(SettingsPath);
        }

        if (!Settings::LogDirectoryPath.empty())
        {
            dirPath = std::filesystem::path(Settings::LogDirectoryPath);

            if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    ("Specified log directory does not exist: " + dirPath.string()).c_str());
                return;
            }
        }
        else
        {
            char documentsPath[MAX_PATH];
            if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
                    "Failed to get path to user's Documents folder.");
                return;
            }

            dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
                "arcdps" / "arcdps.cbtlogs";
        }

        // Determine if we're running under Wine
        bool runningUnderWine = isRunningUnderWine();
        if (runningUnderWine)
        {
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Wine/Proton. Using polling method.");

            parseInitialLogs(processedFiles, numLogsToParse);

            while (!stopMonitoring)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMilliseconds));

                if (stopMonitoring)
                {
                    break;
                }

                scanForNewFiles(dirPath, processedFiles);
            }
        }
        else
        {
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Windows. Using ReadDirectoryChangesW method.");

            directoryMonitor(dirPath, numLogsToParse);
        }
    }
    catch (const std::exception& ex)
    {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
            ("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
    }
}