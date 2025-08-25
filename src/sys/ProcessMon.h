#pragma once
#include <string>
#include <vector>
#include <chrono>

#ifdef _WIN32
#include <pdh.h>
#endif

struct ProcUsage { 
    bool running{false}; 
    double cpu_pct{0}; 
    double mem_mb{0}; 
    std::string name;
    int pid{0};
};

struct SystemMetrics {
    double cpu_pct{0};
    double gpu_pct{0};
    double mem_mb{0};
    double mem_total_mb{0};
    double disk_free_gb{0};
    double disk_total_gb{0};
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    void addProcess(const std::string& name);
    void removeProcess(const std::string& name);
    std::vector<ProcUsage> getProcessStats();
    
    // 시스템 전체 리소스 모니터링
    SystemMetrics getSystemMetrics();
    double getCpuUsage();
    double getGpuUsage();
    double getMemoryUsage();
    
private:
    std::vector<std::string> monitoredProcesses_;
    std::chrono::steady_clock::time_point lastUpdate_;
    std::vector<ProcUsage> lastStats_;
    
#ifdef _WIN32
    PDH_HQUERY cpuQuery_{nullptr};
    PDH_HCOUNTER cpuCounter_{nullptr};
    PDH_HQUERY gpuQuery_{nullptr};
    PDH_HCOUNTER gpuCounter_{nullptr};
#endif
};

ProcUsage QueryProcess(const std::string& name);
