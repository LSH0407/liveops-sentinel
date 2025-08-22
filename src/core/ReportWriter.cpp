#include "ReportWriter.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

namespace core {

ReportWriter::ReportWriter(const ReportConfig& config)
    : config_(config) {
    if (config_.enable) {
        start();
    }
}

ReportWriter::~ReportWriter() {
    stop();
}

void ReportWriter::addSnapshot(const MetricSnapshot& snapshot) {
    if (!config_.enable) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    snapshots_.push_back(snapshot);
}

void ReportWriter::addSnapshot(double rtt, double loss, double dropped, double render, 
                              double cpu, double gpu, double mem) {
    addSnapshot(MetricSnapshot(rtt, loss, dropped, render, cpu, gpu, mem));
}

void ReportWriter::flushNow() {
    if (!config_.enable) return;
    
    should_flush_.store(true);
}

void ReportWriter::start() {
    if (running_.load()) return;
    
    running_.store(true);
    flush_thread_ = std::thread(&ReportWriter::flushThread, this);
}

void ReportWriter::stop() {
    if (!running_.load()) return;
    
    running_.store(false);
    should_flush_.store(true);
    
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
}

std::vector<std::string> ReportWriter::getRecentReportFiles() const {
    std::vector<std::string> files;
    
    try {
        ensureDirectoryExists();
        std::filesystem::path dirPath(config_.dir);
        
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".csv" || ext == ".json") {
                    files.push_back(entry.path().filename().string());
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(files.begin(), files.end(), [&](const std::string& a, const std::string& b) {
            auto pathA = dirPath / a;
            auto pathB = dirPath / b;
            return std::filesystem::last_write_time(pathA) > std::filesystem::last_write_time(pathB);
        });
        
        // Limit to 20 most recent files
        if (files.size() > 20) {
            files.resize(20);
        }
    } catch (const std::exception&) {
        // Return empty vector on error
    }
    
    return files;
}

bool ReportWriter::openReportsFolder() const {
    try {
        ensureDirectoryExists();
        
#ifdef _WIN32
        std::wstring wpath = std::filesystem::path(config_.dir).wstring();
        HINSTANCE result = ShellExecuteW(nullptr, L"open", wpath.c_str(), 
                                        nullptr, nullptr, SW_SHOWNORMAL);
        return reinterpret_cast<intptr_t>(result) > 32;
#else
        std::string command = "xdg-open " + config_.dir;
        return system(command.c_str()) == 0;
#endif
    } catch (const std::exception&) {
        return false;
    }
}

void ReportWriter::setConfig(const ReportConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    if (config_.enable && !running_.load()) {
        start();
    } else if (!config_.enable && running_.load()) {
        stop();
    }
}

ReportConfig ReportWriter::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void ReportWriter::flushThread() {
    while (running_.load()) {
        if (should_flush_.load()) {
            std::vector<MetricSnapshot> snapshotsCopy;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                snapshotsCopy = snapshots_;
                snapshots_.clear();
            }
            
            if (!snapshotsCopy.empty()) {
                std::string csvPath = generateFilename(".csv");
                std::string jsonPath = generateFilename(".json");
                
                // 파일 크기 체크 및 롤오버
                if (shouldRolloverFile(csvPath)) {
                    csvPath = generateFilename("_part2.csv");
                }
                if (shouldRolloverFile(jsonPath)) {
                    jsonPath = generateFilename("_part2.json");
                }
                
                writeCsvFile(csvPath);
                writeJsonFile(jsonPath);
            }
            
            should_flush_.store(false);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string ReportWriter::generateFilename(const std::string& extension) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "metrics_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M") << extension;
    
    return (std::filesystem::path(config_.dir) / ss.str()).string();
}

bool ReportWriter::writeCsvFile(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        // Write header
        file << "ts,rtt_ms,loss_pct,obs_dropped_ratio,avg_render_ms,cpu_pct,gpu_pct,mem_mb\n";
        
        // Write data
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& snapshot : snapshots_) {
            auto time_t = std::chrono::system_clock::to_time_t(snapshot.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                snapshot.timestamp.time_since_epoch()).count() % 1000;
            
            file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                 << "." << std::setfill('0') << std::setw(3) << ms << ","
                 << snapshot.rtt_ms << ","
                 << snapshot.loss_pct << ","
                 << snapshot.obs_dropped_ratio << ","
                 << snapshot.avg_render_ms << ","
                 << snapshot.cpu_pct << ","
                 << snapshot.gpu_pct << ","
                 << snapshot.mem_mb << "\n";
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ReportWriter::writeJsonFile(const std::string& path) const {
    try {
        nlohmann::json j;
        j["metadata"] = {
            {"exportTime", std::chrono::system_clock::now().time_since_epoch().count()},
            {"totalSnapshots", snapshots_.size()},
            {"flushIntervalSec", config_.flushIntervalSec}
        };
        
        nlohmann::json snapshotsArray = nlohmann::json::array();
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& snapshot : snapshots_) {
            nlohmann::json snapshotJson;
            snapshotJson["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                snapshot.timestamp.time_since_epoch()).count();
            snapshotJson["rtt_ms"] = snapshot.rtt_ms;
            snapshotJson["loss_pct"] = snapshot.loss_pct;
            snapshotJson["obs_dropped_ratio"] = snapshot.obs_dropped_ratio;
            snapshotJson["avg_render_ms"] = snapshot.avg_render_ms;
            snapshotJson["cpu_pct"] = snapshot.cpu_pct;
            snapshotJson["gpu_pct"] = snapshot.gpu_pct;
            snapshotJson["mem_mb"] = snapshot.mem_mb;
            snapshotsArray.push_back(snapshotJson);
        }
        
        j["snapshots"] = snapshotsArray;
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void ReportWriter::ensureDirectoryExists() const {
    try {
        std::filesystem::create_directories(config_.dir);
    } catch (const std::exception&) {
        // Directory creation failed, but we'll continue
    }
}

bool ReportWriter::shouldRolloverFile(const std::string& path) const {
    try {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        
        auto fileSize = std::filesystem::file_size(path);
        auto maxSize = static_cast<std::uintmax_t>(config_.maxFileSizeMB) * 1024 * 1024;
        
        return fileSize >= maxSize;
    } catch (const std::exception&) {
        return false;
    }
}

std::string ReportWriter::sanitizeForPrivacy(const std::string& data) const {
    // PII(개인식별정보) 제거
    std::string sanitized = data;
    
    // 사용자명/호스트명 패턴 제거
    // 실제 구현에서는 더 정교한 패턴 매칭 필요
    
    // Windows 사용자명 패턴 제거
    size_t pos = 0;
    while ((pos = sanitized.find("C:\\Users\\", pos)) != std::string::npos) {
        size_t endPos = sanitized.find("\\", pos + 9);
        if (endPos != std::string::npos) {
            sanitized.replace(pos + 9, endPos - pos - 9, "[USERNAME]");
        }
        pos += 9;
    }
    
    // 호스트명 패턴 제거 (예: DESKTOP-XXXXX)
    std::regex hostnamePattern(R"((DESKTOP|LAPTOP|PC)-[A-Z0-9]+)");
    sanitized = std::regex_replace(sanitized, hostnamePattern, "[HOSTNAME]");
    
    return sanitized;
}

} // namespace core
