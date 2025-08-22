#pragma once
#include <deque>
#include <vector>
#include <chrono>
#include <mutex>

class EMA {
public:
  explicit EMA(double alpha=0.2):alpha_(alpha){}
  double push(double x){ v_ = init_? alpha_*x + (1-alpha_)*v_ : x; init_=true; return v_; }
  double value() const { return v_; }
private:
  double alpha_{};
  double v_{0}; bool init_{false};
};

struct MetricSample {
    double value;
    std::chrono::steady_clock::time_point timestamp;
};

class MetricsCollector {
public:
    MetricsCollector(size_t maxSamples = 1000);
    
    void addSample(double value);
    double getAverage() const;
    double getMin() const;
    double getMax() const;
    double getStdDev() const;
    std::vector<double> getRecentSamples(size_t count) const;
    size_t getSampleCount() const;
    void clear();
    
private:
    mutable std::mutex mutex_;
    std::deque<MetricSample> samples_;
    size_t maxSamples_;
    
    double sum_;
    double sumSquares_;
    double min_;
    double max_;
    bool initialized_;
};
