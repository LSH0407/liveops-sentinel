#include "Probe.h"
#include <asio.hpp>
#include <chrono>
#include <vector>
#include <random>
using asio::ip::udp;

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <regex>

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

class ProbeImpl {
public:
    ProbeImpl() : last_check_time_(std::chrono::steady_clock::now()) {
        initializeNetworkCounters();
    }
    
    ~ProbeImpl() = default;
    
    json getMetrics() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_check_time_).count();
        
        // ?ㅽ듃?뚰겕 移댁슫???낅뜲?댄듃
        updateNetworkCounters();
        
        // ???룺 怨꾩궛
        double bandwidth_mbps = calculateBandwidth(duration);
        
        // RTT 痢≪젙
        double rtt_ms = measureRTT();
        
        // ?⑦궥 ?먯떎 痢≪젙
        double loss_pct = measurePacketLoss();
        
        json metrics;
        metrics["rtt_ms"] = rtt_ms;
        metrics["loss_pct"] = loss_pct;
        metrics["bandwidth_mbps"] = bandwidth_mbps;
        metrics["bytes_sent"] = bytes_sent_;
        metrics["bytes_received"] = bytes_received_;
        
        last_check_time_ = now;
        return metrics;
    }
    
private:
    std::chrono::steady_clock::time_point last_check_time_;
    uint64_t bytes_sent_{0};
    uint64_t bytes_received_{0};
    uint64_t last_bytes_sent_{0};
    uint64_t last_bytes_received_{0};
    
#ifdef _WIN32
    MIB_IFROW interface_info_;
    
    void initializeNetworkCounters() {
        // 湲곕낯 ?ㅽ듃?뚰겕 ?명꽣?섏씠???뺣낫 珥덇린??
        memset(&interface_info_, 0, sizeof(interface_info_));
        interface_info_.dwIndex = 1; // 泥?踰덉㎏ ?명꽣?섏씠??
    }
    
    void updateNetworkCounters() {
        MIB_IFROW ifRow;
        memset(&ifRow, 0, sizeof(ifRow));
        ifRow.dwIndex = interface_info_.dwIndex;
        
        if (GetIfEntry(&ifRow) == NO_ERROR) {
            last_bytes_sent_ = bytes_sent_;
            last_bytes_received_ = bytes_received_;
            bytes_sent_ = ifRow.dwOutOctets;
            bytes_received_ = ifRow.dwInOctets;
        }
    }
#else
    std::string interface_name_;
    
    void initializeNetworkCounters() {
        // 湲곕낯 ?ㅽ듃?뚰겕 ?명꽣?섏씠??李얘린
        interface_name_ = getDefaultInterface();
    }
    
    void updateNetworkCounters() {
        std::string stats_file = "/sys/class/net/" + interface_name_ + "/statistics/";
        
        // ?≪떊 諛붿씠???쎄린
        std::ifstream tx_file(stats_file + "tx_bytes");
        if (tx_file.is_open()) {
            uint64_t current_tx;
            tx_file >> current_tx;
            last_bytes_sent_ = bytes_sent_;
            bytes_sent_ = current_tx;
            tx_file.close();
        }
        
        // ?섏떊 諛붿씠???쎄린
        std::ifstream rx_file(stats_file + "rx_bytes");
        if (rx_file.is_open()) {
            uint64_t current_rx;
            rx_file >> current_rx;
            last_bytes_received_ = bytes_received_;
            bytes_received_ = current_rx;
            rx_file.close();
        }
    }
    
    std::string getDefaultInterface() {
        // 湲곕낯 寃뚯씠?몄썾???명꽣?섏씠??李얘린
        FILE* pipe = popen("ip route | grep default | awk '{print $5}' | head -1", "r");
        if (!pipe) return "eth0";
        
        char buffer[128];
        std::string result = "";
        
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr)
                result += buffer;
        }
        
        pclose(pipe);
        
        // 媛쒗뻾 臾몄옄 ?쒓굅
        if (!result.empty() && result[result.length()-1] == '\n') {
            result.erase(result.length()-1);
        }
        
        return result.empty() ? "eth0" : result;
    }
#endif
    
    double calculateBandwidth(double duration_ms) {
        if (duration_ms <= 0) return 0.0;
        
        uint64_t bytes_sent_diff = bytes_sent_ - last_bytes_sent_;
        uint64_t bytes_received_diff = bytes_received_ - last_bytes_received_;
        uint64_t total_bytes = bytes_sent_diff + bytes_received_diff;
        
        // Mbps濡?蹂??(諛붿씠??-> 鍮꾪듃 -> Mbps)
        double duration_seconds = duration_ms / 1000.0;
        double bandwidth_bps = (total_bytes * 8.0) / duration_seconds;
        double bandwidth_mbps = bandwidth_bps / 1000000.0;
        
        return bandwidth_mbps;
    }
    
    double measureRTT() {
        // 媛꾨떒??ping ?쒕??덉씠??(?ㅼ젣濡쒕뒗 ???뺢탳??援ы쁽 ?꾩슂)
        // ?ш린?쒕뒗 湲곕낯媛?諛섑솚
        return 20.0 + (rand() % 30); // 20-50ms 踰붿쐞???쒕뜡 媛?
    }
    
    double measurePacketLoss() {
        // ?⑦궥 ?먯떎 痢≪젙 (?ㅼ젣濡쒕뒗 ???뺢탳??援ы쁽 ?꾩슂)
        // ?ш린?쒕뒗 湲곕낯媛?諛섑솚
        return 0.1 + (rand() % 10) / 100.0; // 0.1-1.0% 踰붿쐞???쒕뜡 媛?
    }
};

// Probe 援ы쁽
Probe::Probe() : impl_(std::make_unique<ProbeImpl>()) {}

Probe::~Probe() = default;

json Probe::getMetrics() {
