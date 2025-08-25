#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <atomic>

struct BandwidthResult {
    double upload_mbps{0.0};
    double download_mbps{0.0};
    double latency_ms{0.0};
    bool success{false};
    std::string error_message;
};

class BandwidthTest {
public:
    BandwidthTest();
    ~BandwidthTest();
    
    // 대역폭 테스트 실행
    BandwidthResult runTest(const std::string& server = "speed.cloudflare.com", int duration_seconds = 10);
    
    // 비동기 테스트 (콜백으로 결과 전달)
    void runTestAsync(const std::string& server, int duration_seconds, 
                     std::function<void(const BandwidthResult&)> callback);
    
    // 테스트 중단
    void stopTest();
    
    // 진행률 확인
    double getProgress() const;
    
private:
    std::atomic<bool> running_{false};
    std::atomic<double> progress_{0.0};
    
    // HTTP 기반 대역폭 측정
    BandwidthResult measureUploadBandwidth(const std::string& server, int duration_seconds);
    BandwidthResult measureDownloadBandwidth(const std::string& server, int duration_seconds);
    double measureLatency(const std::string& server);
    
    // 데이터 생성
    std::string generateTestData(size_t size_bytes);
};

