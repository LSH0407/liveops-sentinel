#include "Metrics.h"
#include <algorithm>
#include <cmath>
#include <numeric>

MetricsCollector::MetricsCollector(size_t maxSamples) 
    : maxSamples_(maxSamples), sum_(0.0), sumSquares_(0.0), 
      min_(0.0), max_(0.0), initialized_(false) {
}

void MetricsCollector::addSample(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    MetricSample sample{value, std::chrono::steady_clock::now()};
    samples_.push_back(sample);
    
    if (samples_.size() > maxSamples_) {
        const auto& oldest = samples_.front();
        sum_ -= oldest.value;
        sumSquares_ -= oldest.value * oldest.value;
        samples_.pop_front();
    }
    
    sum_ += value;
    sumSquares_ += value * value;
    
    if (!initialized_) {
        min_ = max_ = value;
        initialized_ = true;
    } else {
        min_ = std::min(min_, value);
        max_ = std::max(max_, value);
    }
}

double MetricsCollector::getAverage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_.empty() ? 0.0 : sum_ / samples_.size();
}

double MetricsCollector::getMin() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_ ? min_ : 0.0;
}

double MetricsCollector::getMax() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_ ? max_ : 0.0;
}

double MetricsCollector::getStdDev() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.size() < 2) return 0.0;
    
    double mean = sum_ / samples_.size();
    double variance = (sumSquares_ / samples_.size()) - (mean * mean);
    return std::sqrt(std::max(0.0, variance));
}

std::vector<double> MetricsCollector::getRecentSamples(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<double> result;
    
    size_t start = (samples_.size() > count) ? samples_.size() - count : 0;
    for (size_t i = start; i < samples_.size(); ++i) {
        result.push_back(samples_[i].value);
    }
    
    return result;
}

size_t MetricsCollector::getSampleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_.size();
}

void MetricsCollector::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
    sum_ = 0.0;
    sumSquares_ = 0.0;
    min_ = max_ = 0.0;
    initialized_ = false;
}
