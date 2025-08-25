#include "ProcessMon.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <vector>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#else
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstring>
#endif

ProcessMonitor::ProcessMonitor() : lastUpdate_(std::chrono::steady_clock::now()) {
#ifdef _WIN32
    // PDH 초기화
    PdhOpenQuery(nullptr, 0, &cpuQuery_);
    PdhAddCounterW(cpuQuery_, L"\\Processor(_Total)\\% Processor Time", 0, &cpuCounter_);
    PdhCollectQueryData(cpuQuery_);
    
    // GPU 카운터 초기화 (NVIDIA)
    PdhOpenQuery(nullptr, 0, &gpuQuery_);
    PdhAddCounterW(gpuQuery_, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpuCounter_);
    PdhCollectQueryData(gpuQuery_);
#endif
}

ProcessMonitor::~ProcessMonitor() {
#ifdef _WIN32
    if (cpuQuery_) PdhCloseQuery(cpuQuery_);
    if (gpuQuery_) PdhCloseQuery(gpuQuery_);
#endif
}

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

// 시스템 전체 리소스 모니터링 메서드들 추가
SystemMetrics ProcessMonitor::getSystemMetrics() {
    SystemMetrics metrics;
    
#ifdef _WIN32
    // CPU 사용률 측정
    PDH_FMT_COUNTERVALUE cpuValue;
    PdhCollectQueryData(cpuQuery_);
    PdhGetFormattedCounterValue(cpuCounter_, PDH_FMT_DOUBLE, nullptr, &cpuValue);
    metrics.cpu_pct = cpuValue.doubleValue;
    
    // GPU 사용률 측정 (간단한 방법)
    PDH_FMT_COUNTERVALUE gpuValue;
    PdhCollectQueryData(gpuQuery_);
    PdhGetFormattedCounterValue(gpuCounter_, PDH_FMT_DOUBLE, nullptr, &gpuValue);
    metrics.gpu_pct = gpuValue.doubleValue;
    
    // 메모리 사용량 측정
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    metrics.mem_mb = static_cast<double>(memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024);
    metrics.mem_total_mb = static_cast<double>(memInfo.ullTotalPhys) / (1024 * 1024);
    
    // 디스크 사용량 측정
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        metrics.disk_free_gb = static_cast<double>(freeBytesAvailable.QuadPart) / (1024 * 1024 * 1024);
        metrics.disk_total_gb = static_cast<double>(totalBytes.QuadPart) / (1024 * 1024 * 1024);
    }
    
#else
    // Linux 시스템 모니터링
    std::ifstream statFile("/proc/stat");
    if (statFile.is_open()) {
        std::string line;
        std::getline(statFile, line);
        statFile.close();
        
        // CPU 사용률 계산 (간단한 방법)
        metrics.cpu_pct = 25.0; // 임시값
    }
    
    // 메모리 사용량
    std::ifstream meminfoFile("/proc/meminfo");
    if (meminfoFile.is_open()) {
        std::string line;
        unsigned long total = 0, available = 0;
        
        while (std::getline(meminfoFile, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                sscanf(line.c_str(), "MemTotal: %lu", &total);
            } else if (line.find("MemAvailable:") != std::string::npos) {
                sscanf(line.c_str(), "MemAvailable: %lu", &available);
                break;
            }
        }
        meminfoFile.close();
        
        if (total > 0) {
            metrics.mem_mb = static_cast<double>(total - available) / 1024.0;
            metrics.mem_total_mb = static_cast<double>(total) / 1024.0;
        }
    }
    
    // GPU 사용률 (Linux에서는 nvidia-smi 사용)
    metrics.gpu_pct = 15.0; // 임시값
#endif
    
    return metrics;
}

double ProcessMonitor::getCpuUsage() {
    return getSystemMetrics().cpu_pct;
}

double ProcessMonitor::getGpuUsage() {
    return getSystemMetrics().gpu_pct;
}

double ProcessMonitor::getMemoryUsage() {
    return getSystemMetrics().mem_mb;
}
