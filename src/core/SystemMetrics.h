#pragma once
#include <string>
#include <map>
#include <memory>

namespace core {

class SystemMetrics {
public:
    static SystemMetrics& getInstance();
    
    SystemMetrics();
    ~SystemMetrics();
    
    // 시스템 메트릭 수집
    std::map<std::string, double> getMetrics();
    
    // 개별 메트릭 조회
    double getCpuUsage();
    double getMemoryUsage();
    double getDiskUsage();
    
private:
    SystemMetrics(const SystemMetrics&) = delete;
    SystemMetrics& operator=(const SystemMetrics&) = delete;
    
    class SystemMetricsImpl;
    std::unique_ptr<SystemMetricsImpl> impl_;
};

} // namespace core
