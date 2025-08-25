#include "Probe.h"
#include <chrono>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#endif

namespace net {

class Probe::ProbeImpl {
public:
    ProbeImpl() : last_check_time_(std::chrono::steady_clock::now()) {
        initializeNetworkCounters();
    }
    
    ~ProbeImpl() = default;
    
    std::map<std::string, double> getMetrics() {
        auto now = std::chrono::steady_clock::now();
        
        // 네트워크 카운터 업데이트
        updateNetworkCounters();
        
        std::map<std::string, double> metrics;
        metrics["rtt_ms"] = getRttMs();
        metrics["loss_pct"] = getLossPercent();
        metrics["uplink_kbps"] = getUplinkKbps();
        
        last_check_time_ = now;
        return metrics;
    }
    
    double getRttMs() {
        // 간단한 RTT 측정 (실제로는 ping 명령어나 TCP connect 사용)
        // 여기서는 시뮬레이션된 값 반환
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::normal_distribution<> d(50.0, 10.0); // 평균 50ms, 표준편차 10ms
        
        return std::fmax(1.0, d(gen));
    }
    
    double getLossPercent() {
        // 간단한 패킷 손실 측정 (실제로는 ping 통계 사용)
        // 여기서는 시뮬레이션된 값 반환
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::exponential_distribution<> d(0.01); // 평균 1% 손실
        
        return std::fmin(100.0, d(gen));
    }
    
    double getUplinkKbps() {
        // 업링크 대역폭 측정 (실제로는 speedtest-cli나 네트워크 카운터 사용)
        // 여기서는 시뮬레이션된 값 반환
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::normal_distribution<> d(10000.0, 2000.0); // 평균 10Mbps, 표준편차 2Mbps
        
        return std::fmax(100.0, d(gen));
    }
    
    void setProbeHosts(const std::vector<std::string>& hosts) {
        probe_hosts_ = hosts;
    }
    
    void setProbeInterval(std::chrono::milliseconds interval) {
        probe_interval_ = interval;
    }
    
private:
    std::chrono::steady_clock::time_point last_check_time_;
    std::vector<std::string> probe_hosts_{"8.8.8.8", "1.1.1.1", "208.67.222.222"};
    std::chrono::milliseconds probe_interval_{1000};
    
    // 네트워크 카운터 (Windows)
#ifdef _WIN32
    ULONGLONG last_bytes_sent_{0};
    ULONGLONG last_bytes_recv_{0};
    ULONGLONG last_packets_sent_{0};
    ULONGLONG last_packets_recv_{0};
    
    void initializeNetworkCounters() {
        updateNetworkCounters();
    }
    
    void updateNetworkCounters() {
        MIB_IFROW ifRow;
        memset(&ifRow, 0, sizeof(ifRow));
        ifRow.dwIndex = 1; // 첫 번째 네트워크 인터페이스
        
        if (GetIfEntry(&ifRow) == NO_ERROR) {
            last_bytes_sent_ = ifRow.dwOutOctets;
            last_bytes_recv_ = ifRow.dwInOctets;
            last_packets_sent_ = ifRow.dwOutUcastPkts;
            last_packets_recv_ = ifRow.dwInUcastPkts;
        }
    }
#else
    void initializeNetworkCounters() {
        // Linux에서는 /proc/net/dev 사용
    }
    
    void updateNetworkCounters() {
        // Linux에서는 /proc/net/dev 사용
    }
#endif
};

// Probe 구현
Probe& Probe::getInstance() {
    static Probe instance;
    return instance;
}

Probe::Probe() : impl_(std::make_unique<ProbeImpl>()) {}

Probe::~Probe() = default;

std::map<std::string, double> Probe::getMetrics() {
    return impl_->getMetrics();
}

double Probe::getRttMs() {
    return impl_->getRttMs();
}

double Probe::getLossPercent() {
    return impl_->getLossPercent();
}

double Probe::getUplinkKbps() {
    return impl_->getUplinkKbps();
}

void Probe::setProbeHosts(const std::vector<std::string>& hosts) {
    impl_->setProbeHosts(hosts);
}

void Probe::setProbeInterval(std::chrono::milliseconds interval) {
    impl_->setProbeInterval(interval);
}

} // namespace net 