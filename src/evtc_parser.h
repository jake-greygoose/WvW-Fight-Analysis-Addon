#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <mutex>
#include <atomic>


// Function declarations


void parseInitialLogs();
std::vector<char> extractZipFile(const std::string& filePath);
void processNewEVTCFile(const std::string& filePath);


// Extern declarations for global variables

extern int currentLogIndex;
extern std::mutex parsedLogsMutex;
extern std::atomic<bool> initialParsingComplete;