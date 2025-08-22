#pragma once
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <asio.hpp>

enum class BenchProtocol {
    UDP,
    TCP
};

enum class BenchMode {
    SERVER,
    CLIENT
};

struct SystemMetrics {
    double cpuPct{0.0};
    double gpuPct{0.0};
    double memPct{0.0};
    double diskWriteMBps{0.0};
    std::chrono::steady_clock::time_point timestamp;
};

struct BenchResult {
    double uplinkMbps{0.0};
    double downlinkMbps{0.0};
    double lossPct{0.0};
    double rttMsAvg{0.0};
    double rttMsMin{0.0};
    double rttMsMax{0.0};
    double jitterMs{0.0};
    int totalPackets{0};
    int receivedPackets{0};
    std::chrono::steady_clock::time_point timestamp;
    
    // 시스템 지표 추가
    SystemMetrics systemMetrics;
};

struct BenchConfig {
    BenchProtocol protocol{BenchProtocol::UDP};
    BenchMode mode{BenchMode::CLIENT};
    std::string targetHost{"127.0.0.1"};
    int targetPort{50052};
    int durationSec{30};
    int packetSize{1024};
    int packetsPerSec{1000};
    bool collectSystemMetrics{true};
};

using BenchCallback = std::function<void(const BenchResult&)>;

class BandwidthBench {
public:
    BandwidthBench();
    ~BandwidthBench();
    
    bool start(const BenchConfig& config, BenchCallback callback);
    void stop();
    bool isRunning() const;
    
    // 서버 모드에서 현재 수신 통계
    BenchResult getServerStats() const;
    
    // 시스템 지표 수집
    SystemMetrics getCurrentSystemMetrics() const;
    
private:
    void runServer();
    void runClient();
    void runUdpServer();
    void runTcpServer();
    void runUdpClient();
    void runTcpClient();
    void runSystemMetricsCollector();
    
    void updateStats(const BenchResult& result);
    SystemMetrics collectSystemMetrics();
    
    std::unique_ptr<asio::io_context> io_context_;
    std::unique_ptr<asio::ip::tcp::acceptor> tcp_acceptor_;
    std::unique_ptr<asio::ip::udp::socket> udp_socket_;
    
    BenchConfig config_;
    BenchCallback callback_;
    std::atomic<bool> running_{false};
    std::thread bench_thread_;
    std::thread system_metrics_thread_;
    
    // 서버 모드 통계
    mutable std::mutex server_stats_mutex_;
    BenchResult server_stats_;
    std::vector<std::chrono::steady_clock::time_point> packet_timestamps_;
    
    // 클라이언트 모드 통계
    std::vector<double> rtt_samples_;
    std::vector<int64_t> sent_timestamps_;
    std::vector<int64_t> received_timestamps_;
    
    // 시스템 지표
    mutable std::mutex system_metrics_mutex_;
    SystemMetrics current_system_metrics_;
};
