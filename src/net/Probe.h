#pragma once
#include <functional>
#include <thread>
#include <atomic>
#include <string>

#include <vector>
#include <chrono>
#include <mutex>

struct ProbeSample {
    double rtt_ms;
    double loss_pct;
    std::chrono::steady_clock::time_point timestamp;
    
    ProbeSample(double rtt, double loss) 
        : rtt_ms(rtt), loss_pct(loss), timestamp(std::chrono::steady_clock::now()) {}
};

class Probe {
public:
  using Callback = std::function<void(double rtt_ms, double loss_pct)>;
  bool start(const std::string& host, int port, int rateHz, Callback cb);
  void stop();
  
  // Recent metrics API
  std::vector<ProbeSample> getRecentSamples(int seconds) const;
  double getAverageRtt(int seconds) const;
  double getAverageLoss(int seconds) const;
  double getMaxRtt(int seconds) const;
  double getMaxLoss(int seconds) const;
  
  // 로컬 테스트용 UDP 에코 서버 실행/중지
  static bool startLocalEcho(int port);
  static void stopLocalEcho();
private:
  std::thread th_; std::atomic<bool> run_{false};
  Callback cb_;
  static std::thread echoTh_; static std::atomic<bool> echoRun_;
  
  // Recent samples storage
  mutable std::mutex samplesMutex_;
  std::vector<ProbeSample> recentSamples_;
  static constexpr size_t MAX_SAMPLES = 600; // 10 minutes at 1 sample per second
};
