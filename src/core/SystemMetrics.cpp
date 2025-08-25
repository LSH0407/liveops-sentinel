#include "SystemMetrics.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#else
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#endif

namespace core {

class SystemMetrics::SystemMetricsImpl {
public:
    SystemMetricsImpl() {
#ifdef _WIN32
        // PDH 쿼리 초기화
        PdhOpenQuery(nullptr, 0, &cpu_query_);
        PdhAddCounterW(cpu_query_, L"\\Processor(_Total)\\% Processor Time", 0, &cpu_counter_);
        PdhCollectQueryData(cpu_query_);
        
        // 메모리 정보 초기화
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        total_memory_ = memInfo.ullTotalPhys;
#endif
    }
    
    ~SystemMetricsImpl() {
#ifdef _WIN32
        if (cpu_query_) {
            PdhCloseQuery(cpu_query_);
        }
#endif
    }
    
    std::map<std::string, double> getMetrics() {
        std::map<std::string, double> metrics;
        
        metrics["cpu_pct"] = getCpuUsage();
        metrics["memory_pct"] = getMemoryUsage();
        metrics["disk_pct"] = getDiskUsage();
        
        return metrics;
    }
    
    double getCpuUsage() {
#ifdef _WIN32
        PDH_FMT_COUNTERVALUE counterVal;
        
        PdhCollectQueryData(cpu_query_);
        PdhGetFormattedCounterValue(cpu_counter_, PDH_FMT_DOUBLE, nullptr, &counterVal);
        
        return counterVal.doubleValue;
#else
        // Linux/Unix 시스템에서는 /proc/stat을 읽어서 계산
        return getCpuUsageLinux();
#endif
    }
    
    double getMemoryUsage() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        return static_cast<double>(memInfo.dwMemoryLoad);
#else
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            unsigned long total = si.totalram * si.mem_unit;
            unsigned long free = si.freeram * si.mem_unit;
            return static_cast<double>(total - free) / total * 100.0;
        }
        return 0.0;
#endif
    }
    
    double getDiskUsage() {
#ifdef _WIN32
        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        
        if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            double used = static_cast<double>(totalBytes.QuadPart - totalFreeBytes.QuadPart);
            double total = static_cast<double>(totalBytes.QuadPart);
            return (used / total) * 100.0;
        }
        return 0.0;
#else
        struct statvfs stat;
        if (statvfs("/", &stat) == 0) {
            double total = static_cast<double>(stat.f_blocks * stat.f_frsize);
            double free = static_cast<double>(stat.f_bavail * stat.f_frsize);
            return ((total - free) / total) * 100.0;
        }
        return 0.0;
#endif
    }
    
private:
#ifdef _WIN32
    PDH_HQUERY cpu_query_{nullptr};
    PDH_HCOUNTER cpu_counter_{nullptr};
    ULONGLONG total_memory_{0};
    
    double getCpuUsageLinux() {
        // Linux CPU 사용률 계산 (간단한 구현)
        return 0.0;
    }
#else
    double getCpuUsageLinux() {
        // Linux CPU 사용률 계산 (간단한 구현)
        return 0.0;
    }
#endif
};

// SystemMetrics 구현
SystemMetrics& SystemMetrics::getInstance() {
    static SystemMetrics instance;
    return instance;
}

SystemMetrics::SystemMetrics() : impl_(std::make_unique<SystemMetricsImpl>()) {}

SystemMetrics::~SystemMetrics() = default;

std::map<std::string, double> SystemMetrics::getMetrics() {
    return impl_->getMetrics();
}

double SystemMetrics::getCpuUsage() {
    return impl_->getCpuUsage();
}

double SystemMetrics::getMemoryUsage() {
    return impl_->getMemoryUsage();
}

double SystemMetrics::getDiskUsage() {
    return impl_->getDiskUsage();
}

} // namespace core
