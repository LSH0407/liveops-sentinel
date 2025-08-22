#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

namespace core {

struct MetricSnapshot {
    std::chrono::system_clock::time_point timestamp;
    double rtt_ms;
    double loss_pct;
    double obs_dropped_ratio;
    double avg_render_ms;
    double cpu_pct;
    double gpu_pct;
    double mem_mb;
    
    MetricSnapshot() = default;
    MetricSnapshot(double rtt, double loss, double dropped, double render, 
                   double cpu, double gpu, double mem)
        : timestamp(std::chrono::system_clock::now())
        , rtt_ms(rtt)
        , loss_pct(loss)
        , obs_dropped_ratio(dropped)
        , avg_render_ms(render)
        , cpu_pct(cpu)
        , gpu_pct(gpu)
        , mem_mb(mem) {}
};

struct ReportConfig {
    bool enable{true};
    int flushIntervalSec{10};
    std::string dir{"reports"};
    int maxFileSizeMB{25};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReportConfig, enable, flushIntervalSec, dir, maxFileSizeMB)
};

class ReportWriter {
public:
    explicit ReportWriter(const ReportConfig& config);
    ~ReportWriter();
    
    // Thread-safe metric collection
    void addSnapshot(const MetricSnapshot& snapshot);
    void addSnapshot(double rtt, double loss, double dropped, double render, 
                     double cpu, double gpu, double mem);
    
    // Manual control
    void flushNow();
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // File operations
    std::vector<std::string> getRecentReportFiles() const;
    bool openReportsFolder() const;
    
    // Configuration
    void setConfig(const ReportConfig& config);
    ReportConfig getConfig() const;

private:
    void flushThread();
    std::string generateFilename(const std::string& extension) const;
    bool writeCsvFile(const std::string& path) const;
    bool writeJsonFile(const std::string& path) const;
    void ensureDirectoryExists() const;
    bool shouldRolloverFile(const std::string& path) const;
    std::string sanitizeForPrivacy(const std::string& data) const;
    
    ReportConfig config_;
    mutable std::mutex mutex_;
    std::vector<MetricSnapshot> snapshots_;
    std::thread flush_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_flush_{false};
};

} // namespace core
