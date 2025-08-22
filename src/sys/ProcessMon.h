#pragma once
#include <string>
#include <vector>
#include <chrono>

struct ProcUsage { 
    bool running{false}; 
    double cpu_pct{0}; 
    double mem_mb{0}; 
    std::string name;
    int pid{0};
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    void addProcess(const std::string& name);
    void removeProcess(const std::string& name);
    std::vector<ProcUsage> getProcessStats();
    
private:
    std::vector<std::string> monitoredProcesses_;
    std::chrono::steady_clock::time_point lastUpdate_;
    std::vector<ProcUsage> lastStats_;
};

ProcUsage QueryProcess(const std::string& name);
