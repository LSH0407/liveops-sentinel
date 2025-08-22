#include "ProcessMon.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <vector>
#else
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstring>
#endif

ProcessMonitor::ProcessMonitor() : lastUpdate_(std::chrono::steady_clock::now()) {}

ProcessMonitor::~ProcessMonitor() = default;

void ProcessMonitor::addProcess(const std::string& name) {
    if (std::find(monitoredProcesses_.begin(), monitoredProcesses_.end(), name) == monitoredProcesses_.end()) {
        monitoredProcesses_.push_back(name);
        spdlog::info("Added process to monitor: {}", name);
    }
}

void ProcessMonitor::removeProcess(const std::string& name) {
    auto it = std::find(monitoredProcesses_.begin(), monitoredProcesses_.end(), name);
    if (it != monitoredProcesses_.end()) {
        monitoredProcesses_.erase(it);
        spdlog::info("Removed process from monitor: {}", name);
    }
}

std::vector<ProcUsage> ProcessMonitor::getProcessStats() {
    std::vector<ProcUsage> stats;
    for (const auto& name : monitoredProcesses_) {
        stats.push_back(QueryProcess(name));
    }
    return stats;
}

#ifdef _WIN32
ProcUsage QueryProcess(const std::string& name) {
    ProcUsage usage;
    usage.name = name;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return usage;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            std::string processName = pe32.szExeFile;
            if (processName.find(name) != std::string::npos) {
                usage.running = true;
                usage.pid = pe32.th32ProcessID;
                
                // CPU 및 메모리 사용량 측정
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        usage.mem_mb = static_cast<double>(pmc.WorkingSetSize) / (1024 * 1024);
                    }
                    CloseHandle(hProcess);
                }
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return usage;
}
#else
ProcUsage QueryProcess(const std::string& name) {
    ProcUsage usage;
    usage.name = name;
    
    DIR* dir = opendir("/proc");
    if (!dir) {
        return usage;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            std::string procPath = "/proc/" + std::string(entry->d_name);
            std::string commPath = procPath + "/comm";
            
            std::ifstream commFile(commPath);
            if (commFile.is_open()) {
                std::string processName;
                std::getline(commFile, processName);
                commFile.close();
                
                if (processName.find(name) != std::string::npos) {
                    usage.running = true;
                    usage.pid = std::stoi(entry->d_name);
                    
                    // 메모리 사용량 측정
                    std::string statmPath = procPath + "/statm";
                    std::ifstream statmFile(statmPath);
                    if (statmFile.is_open()) {
                        unsigned long pages;
                        statmFile >> pages;
                        statmFile.close();
                        usage.mem_mb = static_cast<double>(pages * 4096) / (1024 * 1024);
                    }
                    break;
                }
            }
        }
    }
    
    closedir(dir);
    return usage;
}
#endif
