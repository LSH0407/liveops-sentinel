#include "Probe.h"
#include <asio.hpp>
#include <chrono>
#include <vector>
#include <random>
using asio::ip::udp;

std::thread Probe::echoTh_;
std::atomic<bool> Probe::echoRun_{false};

bool Probe::start(const std::string& host, int port, int rateHz, Callback cb){
  stop(); cb_ = std::move(cb); run_ = true;
  th_ = std::thread([=]{
    asio::io_context io;
    udp::resolver res(io);
    udp::endpoint ep = *res.resolve(udp::v4(), host, std::to_string(port)).begin();
    udp::socket sock(io); sock.open(udp::v4());
    sock.non_blocking(true);
    std::vector<char> buf(64);
    int sent=0, recv=0; auto lastReport=std::chrono::steady_clock::now();
    auto interval = std::chrono::milliseconds(1000 / std::max(1,rateHz));
    while(run_){
      auto t0 = std::chrono::steady_clock::now();
      // payload: timestamp ns
      int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t0.time_since_epoch()).count();
      memcpy(buf.data(), &ns, sizeof(ns));
      asio::error_code ec;
      sock.send_to(asio::buffer(buf), ep, 0, ec);
      sent++;

      // wait + try receive
      std::this_thread::sleep_for(interval);
      udp::endpoint from;
      size_t n = sock.receive_from(asio::buffer(buf), from, 0, ec);
      if (!ec && n>=sizeof(ns)){
        int64_t ns2; memcpy(&ns2, buf.data(), sizeof(ns2));
        auto t1 = std::chrono::steady_clock::now();
        double rtt = std::chrono::duration<double, std::milli>(t1 - t0).count();
        recv++;
        // 1초마다 loss 계산 콜백
        auto now = std::chrono::steady_clock::now();
        if (now - lastReport >= std::chrono::seconds(1)){
          double loss = sent>0? (1.0 - (double)recv/(double)sent)*100.0 : 100.0;
          if (cb_) cb_(rtt, loss);
          
          // Store sample for recent metrics API
          {
            std::lock_guard<std::mutex> lock(samplesMutex_);
            recentSamples_.emplace_back(rtt, loss);
            if (recentSamples_.size() > MAX_SAMPLES) {
              recentSamples_.erase(recentSamples_.begin());
            }
          }
          
          sent=recv=0; lastReport = now;
        }
      }
    }
  });
  return true;
}

void Probe::stop(){
  if (run_){ run_ = false; if (th_.joinable()) th_.join(); }
}

bool Probe::startLocalEcho(int port){
  stopLocalEcho(); echoRun_ = true;
  echoTh_ = std::thread([=]{
    asio::io_context io;
    udp::socket sock(io, udp::endpoint(udp::v4(), port));
    std::vector<char> buf(1024);
    while(echoRun_){
      udp::endpoint from; asio::error_code ec;
      size_t n = sock.receive_from(asio::buffer(buf), from, 0, ec);
      if (!ec) sock.send_to(asio::buffer(buf.data(), n), from, 0, ec);
    }
  });
  return true;
}

void Probe::stopLocalEcho(){
  if (echoRun_){ echoRun_ = false; if (echoTh_.joinable()) echoTh_.join(); }
}

// Recent metrics API implementation
std::vector<ProbeSample> Probe::getRecentSamples(int seconds) const {
    std::lock_guard<std::mutex> lock(samplesMutex_);
    
    auto cutoff = std::chrono::steady_clock::now() - std::chrono::seconds(seconds);
    std::vector<ProbeSample> result;
    
    for (const auto& sample : recentSamples_) {
        if (sample.timestamp >= cutoff) {
            result.push_back(sample);
        }
    }
    
    return result;
}

double Probe::getAverageRtt(int seconds) const {
    auto samples = getRecentSamples(seconds);
    if (samples.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& sample : samples) {
        sum += sample.rtt_ms;
    }
    return sum / samples.size();
}

double Probe::getAverageLoss(int seconds) const {
    auto samples = getRecentSamples(seconds);
    if (samples.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& sample : samples) {
        sum += sample.loss_pct;
    }
    return sum / samples.size();
}

double Probe::getMaxRtt(int seconds) const {
    auto samples = getRecentSamples(seconds);
    if (samples.empty()) return 0.0;
    
    double maxRtt = samples[0].rtt_ms;
    for (const auto& sample : samples) {
        if (sample.rtt_ms > maxRtt) {
            maxRtt = sample.rtt_ms;
        }
    }
    return maxRtt;
}

double Probe::getMaxLoss(int seconds) const {
    auto samples = getRecentSamples(seconds);
    if (samples.empty()) return 0.0;
    
    double maxLoss = samples[0].loss_pct;
    for (const auto& sample : samples) {
        if (sample.loss_pct > maxLoss) {
            maxLoss = sample.loss_pct;
        }
    }
    return maxLoss;
}
