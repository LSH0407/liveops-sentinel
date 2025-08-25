#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <memory>

namespace net {

class Probe {
public:
    static Probe& getInstance();
    
    Probe();
    ~Probe();
    
    // 네트워크 메트릭 수집
    std::map<std::string, double> getMetrics();
    
    // 개별 메트릭 조회
    double getRttMs();
    double getLossPercent();
    double getUplinkKbps();
    
    // 설정
    void setProbeHosts(const std::vector<std::string>& hosts);
    void setProbeInterval(std::chrono::milliseconds interval);
    
private:
    Probe(const Probe&) = delete;
    Probe& operator=(const Probe&) = delete;
    
    class ProbeImpl;
    std::unique_ptr<ProbeImpl> impl_;
};

} // namespace net
