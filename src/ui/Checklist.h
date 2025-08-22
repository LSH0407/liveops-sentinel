#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

// Forward declarations
namespace obs {
    class ObsClient;
}

namespace sys {
    class ProcessMonitor;
}

namespace ui {

enum class CheckStatus {
    PENDING,
    PASSED,
    FAILED,
    WARNING
};

struct CheckItem {
    std::string name;
    std::string description;
    CheckStatus status{CheckStatus::PENDING};
    std::string message;
    std::chrono::steady_clock::time_point lastCheck;
    
    CheckItem(const std::string& itemName, const std::string& itemDesc)
        : name(itemName), description(itemDesc) {}
};

struct PreflightConfig {
    std::vector<std::string> ueProcessHints{"UnrealEditor.exe", "UE4Editor.exe"};
    std::string ndiInputKindHint{"ndi"};
    int diskMinGB{10};
    bool warnIfWifi{true};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PreflightConfig, ueProcessHints, ndiInputKindHint, diskMinGB, warnIfWifi)
};

struct PreflightResult {
    std::chrono::system_clock::time_point timestamp;
    std::vector<CheckItem> checks;
    bool overallPassed{false};
    std::string summary;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PreflightResult, timestamp, checks, overallPassed, summary)
};

class Checklist {
public:
    Checklist(std::shared_ptr<obs::ObsClient> obsClient, 
              std::shared_ptr<sys::ProcessMonitor> processMonitor);
    
    // Main interface
    void runPreflight();
    bool isRunning() const { return running_; }
    const PreflightResult& getLastResult() const { return lastResult_; }
    
    // Configuration
    void setConfig(const PreflightConfig& config);
    PreflightConfig getConfig() const;
    
    // UI helpers
    void draw();
    std::string getStatusText() const;
    std::string getResultJson() const;
    bool saveResultToFile(const std::string& path) const;
    bool copyResultToClipboard() const;

private:
    // Individual checks
    void checkObsConnection();
    void checkObsStatus();
    void checkUeProcess();
    void checkNdiInput();
    void checkDiskSpace();
    void checkNetworkAdapter();
    void checkDiskWriteSpeed();
    
    // Utility functions
    void updateCheckItem(const std::string& name, CheckStatus status, const std::string& message);
    std::string getStatusColor(CheckStatus status) const;
    std::string getStatusIcon(CheckStatus status) const;
    double getDiskFreeSpaceGB(const std::string& path) const;
    bool isWifiConnection() const;
    double measureDiskWriteSpeed(const std::string& path) const;
    std::string getObsRecordingPath() const;
    
    std::shared_ptr<obs::ObsClient> obsClient_;
    std::shared_ptr<sys::ProcessMonitor> processMonitor_;
    PreflightConfig config_;
    PreflightResult lastResult_;
    bool running_{false};
    std::chrono::steady_clock::time_point startTime_;
    
    // Progress tracking
    size_t currentCheck_{0};
    size_t totalChecks_{7}; // Wi-Fi와 디스크 쓰기 속도 체크 추가
};

} // namespace ui
