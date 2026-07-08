#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <Windows.h>
#include <mutex>
#include <atomic>


// Function declarations
void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds);
std::vector<char> extractZipFile(const std::string& filePath);
void processNewEVTCFile(const std::filesystem::path& filePath);


// Extern declarations for global variables

extern int currentLogIndex;
extern std::mutex parsedLogsMutex;
extern std::mutex processedFilesMutex;
extern std::atomic<bool> initialParsingComplete;