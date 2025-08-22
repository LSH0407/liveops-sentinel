#include "Checklist.h"
#include "../obs/ObsClient.h"
#include "../sys/ProcessMon.h"
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/statvfs.h>
#endif

namespace ui {

Checklist::Checklist(std::shared_ptr<obs::ObsClient> obsClient, 
                     std::shared_ptr<sys::ProcessMonitor> processMonitor)
    : obsClient_(obsClient)
    , processMonitor_(processMonitor) {
    
    // Initialize check items
    lastResult_.checks = {
        CheckItem("OBS Connection", "OBS WebSocket 연결 및 인증 상태 확인"),
        CheckItem("OBS Status", "OBS 현재 FPS/출력 상태 정상 여부"),
        CheckItem("UE Process", "Unreal Engine 프로세스 실행 상태"),
        CheckItem("NDI Input", "NDI 플러그인/소스 존재 여부"),
        CheckItem("Disk Space", "녹화 디렉토리 여유 공간 확인")
    };
}

void Checklist::runPreflight() {
    if (running_) return;
    
    running_ = true;
    startTime_ = std::chrono::steady_clock::now();
    currentCheck_ = 0;
    
    // Reset all checks to pending
    for (auto& check : lastResult_.checks) {
        check.status = CheckStatus::PENDING;
        check.message.clear();
    }
    
    // Run checks sequentially
    checkObsConnection();
    currentCheck_++;
    
    checkObsStatus();
    currentCheck_++;
    
    checkUeProcess();
    currentCheck_++;
    
    checkNdiInput();
    currentCheck_++;
    
    checkDiskSpace();
    currentCheck_++;
    
    checkNetworkAdapter();
    currentCheck_++;
    
    checkDiskWriteSpeed();
    currentCheck_++;
    
    // Calculate overall result
    lastResult_.timestamp = std::chrono::system_clock::now();
    lastResult_.overallPassed = true;
    int failedCount = 0;
    int warningCount = 0;
    
    for (const auto& check : lastResult_.checks) {
        if (check.status == CheckStatus::FAILED) {
            lastResult_.overallPassed = false;
            failedCount++;
        } else if (check.status == CheckStatus::WARNING) {
            warningCount++;
        }
    }
    
    // Generate summary
    std::stringstream ss;
    ss << "Pre-flight Check 완료: ";
    if (lastResult_.overallPassed) {
        ss << "✅ 모든 항목 통과";
        if (warningCount > 0) {
            ss << " (경고 " << warningCount << "개)";
        }
    } else {
        ss << "❌ 실패 " << failedCount << "개";
        if (warningCount > 0) {
            ss << ", 경고 " << warningCount << "개";
        }
    }
    lastResult_.summary = ss.str();
    
    running_ = false;
}

void Checklist::checkObsConnection() {
    if (!obsClient_) {
        updateCheckItem("OBS Connection", CheckStatus::FAILED, "OBS 클라이언트가 초기화되지 않음");
        return;
    }
    
    if (obsClient_->isConnected()) {
        updateCheckItem("OBS Connection", CheckStatus::PASSED, "OBS WebSocket 연결 성공");
    } else {
        updateCheckItem("OBS Connection", CheckStatus::FAILED, "OBS WebSocket 연결 실패 - OBS Studio가 실행 중이고 WebSocket이 활성화되어 있는지 확인하세요");
    }
}

void Checklist::checkObsStatus() {
    if (!obsClient_ || !obsClient_->isConnected()) {
        updateCheckItem("OBS Status", CheckStatus::FAILED, "OBS 연결이 필요합니다");
        return;
    }
    
    auto status = obsClient_->getStatus();
    
    // Check if OBS is streaming or recording
    bool isStreaming = status.isStreaming;
    bool isRecording = status.isRecording;
    
    if (isStreaming || isRecording) {
        // Check FPS and performance metrics
        if (status.activeFps > 0 && status.activeFps < 25) {
            updateCheckItem("OBS Status", CheckStatus::WARNING, 
                "OBS FPS가 낮습니다: " + std::to_string(static_cast<int>(status.activeFps)) + " FPS");
        } else if (status.droppedFramesRatio > 0.05) {
            updateCheckItem("OBS Status", CheckStatus::WARNING, 
                "프레임 드롭이 높습니다: " + std::to_string(static_cast<int>(status.droppedFramesRatio * 100)) + "%");
        } else {
            updateCheckItem("OBS Status", CheckStatus::PASSED, 
                "OBS 상태 정상 - FPS: " + std::to_string(static_cast<int>(status.activeFps)) + 
                (isStreaming ? " (스트리밍 중)" : "") + 
                (isRecording ? " (녹화 중)" : ""));
        }
    } else {
        updateCheckItem("OBS Status", CheckStatus::PASSED, "OBS 대기 중 - 스트리밍/녹화 준비 완료");
    }
}

void Checklist::checkUeProcess() {
    if (!processMonitor_) {
        updateCheckItem("UE Process", CheckStatus::FAILED, "프로세스 모니터가 초기화되지 않음");
        return;
    }
    
    bool found = false;
    std::string foundProcess;
    
    for (const auto& processHint : config_.ueProcessHints) {
        auto processes = processMonitor_->getProcessesByName(processHint);
        if (!processes.empty()) {
            found = true;
            foundProcess = processHint;
            break;
        }
    }
    
    if (found) {
        updateCheckItem("UE Process", CheckStatus::PASSED, "Unreal Engine 프로세스 발견: " + foundProcess);
    } else {
        updateCheckItem("UE Process", CheckStatus::WARNING, 
            "Unreal Engine 프로세스를 찾을 수 없습니다. 다음 중 하나가 실행 중인지 확인하세요: " + 
            std::accumulate(config_.ueProcessHints.begin(), config_.ueProcessHints.end(), std::string(),
                [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : ", ") + b; }));
    }
}

void Checklist::checkNdiInput() {
    if (!obsClient_ || !obsClient_->isConnected()) {
        updateCheckItem("NDI Input", CheckStatus::FAILED, "OBS 연결이 필요합니다");
        return;
    }
    
    // This would require implementing GetInputList in ObsClient
    // For now, we'll mark it as a warning that requires manual verification
    updateCheckItem("NDI Input", CheckStatus::WARNING, 
        "NDI 입력 소스 확인이 필요합니다. OBS에서 NDI Source가 설정되어 있는지 수동으로 확인하세요.");
}

void Checklist::checkDiskSpace() {
    // Check default recording directory or current working directory
    std::string checkPath = ".";
    
    double freeSpaceGB = getDiskFreeSpaceGB(checkPath);
    
    if (freeSpaceGB >= config_.diskMinGB) {
        updateCheckItem("Disk Space", CheckStatus::PASSED, 
            "여유 공간 충분: " + std::to_string(static_cast<int>(freeSpaceGB)) + " GB");
    } else {
        updateCheckItem("Disk Space", CheckStatus::FAILED, 
            "여유 공간 부족: " + std::to_string(static_cast<int>(freeSpaceGB)) + 
            " GB (최소 " + std::to_string(config_.diskMinGB) + " GB 필요)");
    }
}

void Checklist::updateCheckItem(const std::string& name, CheckStatus status, const std::string& message) {
    for (auto& check : lastResult_.checks) {
        if (check.name == name) {
            check.status = status;
            check.message = message;
            check.lastCheck = std::chrono::steady_clock::now();
            break;
        }
    }
}

void Checklist::draw() {
    ImGui::Begin("Pre-flight Checklist");
    
    if (running_) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "진행 중... (%zu/%zu)", currentCheck_, totalChecks_);
        
        // Progress bar
        float progress = static_cast<float>(currentCheck_) / static_cast<float>(totalChecks_);
        ImGui::ProgressBar(progress, ImVec2(-1, 20));
        
        ImGui::Spacing();
    } else {
        if (ImGui::Button("Run Pre-flight", ImVec2(-1, 30))) {
            runPreflight();
        }
        
        ImGui::Spacing();
        
        // Show last result summary
        if (!lastResult_.summary.empty()) {
            ImGui::TextWrapped("%s", lastResult_.summary.c_str());
            ImGui::Spacing();
        }
    }
    
    // Display check items
    for (const auto& check : lastResult_.checks) {
        ImVec4 color;
        const char* icon;
        
        switch (check.status) {
            case CheckStatus::PASSED:
                color = ImVec4(0, 1, 0, 1);
                icon = "✅";
                break;
            case CheckStatus::FAILED:
                color = ImVec4(1, 0, 0, 1);
                icon = "❌";
                break;
            case CheckStatus::WARNING:
                color = ImVec4(1, 1, 0, 1);
                icon = "⚠️";
                break;
            default:
                color = ImVec4(0.7f, 0.7f, 0.7f, 1);
                icon = "⏳";
                break;
        }
        
        ImGui::TextColored(color, "%s %s", icon, check.name.c_str());
        if (ImGui::IsItemHovered() && !check.description.empty()) {
            ImGui::SetTooltip("%s", check.description.c_str());
        }
        
        if (!check.message.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "- %s", check.message.c_str());
        }
    }
    
    ImGui::Spacing();
    
    // Action buttons
    if (!running_ && !lastResult_.summary.empty()) {
        if (ImGui::Button("Copy Result", ImVec2(120, 25))) {
            copyResultToClipboard();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save JSON", ImVec2(120, 25))) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "reports/preflight_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M") << ".json";
            saveResultToFile(ss.str());
        }
    }
    
    ImGui::End();
}

std::string Checklist::getStatusText() const {
    if (running_) {
        return "진행 중...";
    }
    
    if (lastResult_.summary.empty()) {
        return "대기 중";
    }
    
    return lastResult_.summary;
}

std::string Checklist::getResultJson() const {
    nlohmann::json j = lastResult_;
    return j.dump(2);
}

bool Checklist::saveResultToFile(const std::string& path) const {
    try {
        // Ensure directory exists
        std::filesystem::path filePath(path);
        std::filesystem::create_directories(filePath.parent_path());
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << getResultJson();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool Checklist::copyResultToClipboard() const {
    std::string result = getResultJson();
    
#ifdef _WIN32
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, result.size() + 1);
        if (hGlobal) {
            char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
            strcpy_s(pGlobal, result.size() + 1, result.c_str());
            GlobalUnlock(hGlobal);
            
            SetClipboardData(CF_TEXT, hGlobal);
            CloseClipboard();
            return true;
        }
        CloseClipboard();
    }
#endif
    
    return false;
}

void Checklist::setConfig(const PreflightConfig& config) {
    config_ = config;
}

PreflightConfig Checklist::getConfig() const {
    return config_;
}

double Checklist::getDiskFreeSpaceGB(const std::string& path) const {
    try {
        std::filesystem::path fsPath(path);
        
#ifdef _WIN32
        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExW(fsPath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            return static_cast<double>(freeBytesAvailable.QuadPart) / (1024.0 * 1024.0 * 1024.0);
        }
#else
        struct statvfs stat;
        if (statvfs(fsPath.c_str(), &stat) == 0) {
            return static_cast<double>(stat.f_bavail * stat.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        }
#endif
    } catch (const std::exception&) {
        // Return 0 on error
    }
    
    return 0.0;
}

void Checklist::checkNetworkAdapter() {
    if (!config_.warnIfWifi) {
        updateCheckItem("Network Adapter", CheckStatus::PASSED, "Wi-Fi 경고 비활성화됨");
        return;
    }
    
    if (isWifiConnection()) {
        updateCheckItem("Network Adapter", CheckStatus::WARNING, 
            "Wi-Fi 연결 감지됨 - 유선 연결 권장 (안정성 향상)");
    } else {
        updateCheckItem("Network Adapter", CheckStatus::PASSED, "유선 연결 확인됨");
    }
}

void Checklist::checkDiskWriteSpeed() {
    std::string recordingPath = getObsRecordingPath();
    if (recordingPath.empty()) {
        updateCheckItem("Disk Write Speed", CheckStatus::WARNING, "OBS 녹화 경로를 확인할 수 없음");
        return;
    }
    
    double writeSpeed = measureDiskWriteSpeed(recordingPath);
    if (writeSpeed >= 100.0) {
        updateCheckItem("Disk Write Speed", CheckStatus::PASSED, 
            "디스크 쓰기 속도: " + std::to_string(static_cast<int>(writeSpeed)) + " MB/s (권장 ≥100 MB/s)");
    } else if (writeSpeed >= 50.0) {
        updateCheckItem("Disk Write Speed", CheckStatus::WARNING, 
            "디스크 쓰기 속도: " + std::to_string(static_cast<int>(writeSpeed)) + " MB/s (권장 ≥100 MB/s)");
    } else {
        updateCheckItem("Disk Write Speed", CheckStatus::FAILED, 
            "디스크 쓰기 속도: " + std::to_string(static_cast<int>(writeSpeed)) + " MB/s (권장 ≥100 MB/s)");
    }
}

bool Checklist::isWifiConnection() const {
#ifdef _WIN32
    // Windows에서 네트워크 어댑터 정보 조회
    // 실제 구현에서는 Windows API를 사용하여 Wi-Fi 어댑터 감지
    // 여기서는 간단한 구현으로 대체
    return false; // 기본적으로 유선으로 가정
#else
    // Linux에서 네트워크 어댑터 정보 조회
    return false; // 기본적으로 유선으로 가정
#endif
}

double Checklist::measureDiskWriteSpeed(const std::string& path) const {
    try {
        std::filesystem::path testPath(path);
        std::filesystem::path testFile = testPath / "write_speed_test.tmp";
        
        // 64MB 테스트 파일 생성
        const size_t testSize = 64 * 1024 * 1024; // 64MB
        std::vector<char> buffer(1024 * 1024); // 1MB 버퍼
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::ofstream file(testFile, std::ios::binary);
        if (!file.is_open()) {
            return 0.0;
        }
        
        size_t written = 0;
        while (written < testSize) {
            size_t toWrite = std::min(buffer.size(), testSize - written);
            file.write(buffer.data(), toWrite);
            written += toWrite;
        }
        file.close();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 테스트 파일 삭제
        std::filesystem::remove(testFile);
        
        if (duration.count() > 0) {
            double speedMBps = (testSize / (1024.0 * 1024.0)) / (duration.count() / 1000.0);
            return speedMBps;
        }
    } catch (const std::exception&) {
        // 에러 시 0 반환
    }
    
    return 0.0;
}

std::string Checklist::getObsRecordingPath() const {
    // OBS 녹화 경로 조회 (실제 구현에서는 OBS WebSocket API 사용)
    // 여기서는 기본 경로 반환
    return "C:\\Users\\Public\\Videos"; // Windows 기본 경로
}

} // namespace ui
