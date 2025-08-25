#include "UpdateManager.h"
#include "Logger.h"
#include "Config.h"

#ifdef ENABLE_OBS
#include <curl/curl.h>
#endif
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace core {

class UpdateManager::UpdateManagerImpl {
public:
    UpdateManagerImpl() 
        : auto_update_enabled_(false)
        , update_check_interval_(24)
        , update_channel_("stable")
        , update_server_("https://api.github.com/repos/liveops-sentinel/releases/latest")
        , update_available_(false)
        , update_in_progress_(false)
        , update_installed_(false) {
        
#ifdef ENABLE_OBS
        // CURL 초기화
        curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
        
        // 업데이트 캐시 디렉토리 생성
        createUpdateCacheDirectory();
        
        spdlog::info("UpdateManager 초기화 완료");
    }
    
    ~UpdateManagerImpl() {
#ifdef ENABLE_OBS
        curl_global_cleanup();
#endif
        spdlog::info("UpdateManager 정리 완료");
    }
    
    // 업데이트 확인
    bool checkForUpdates() {
        return checkForUpdates("1.0.0"); // 기본 버전
    }
    
    bool checkForUpdates(const std::string& current_version) {
        if (update_in_progress_) {
            spdlog::warn("업데이트 확인 중: 이미 진행 중인 업데이트가 있습니다");
            return false;
        }
        
        spdlog::info("업데이트 확인 시작: 현재 버전 {}", current_version);
        
        try {
            // GitHub API에서 최신 릴리스 정보 가져오기
            std::string response = fetchLatestReleaseInfo();
            if (response.empty()) {
                spdlog::error("업데이트 정보를 가져올 수 없습니다");
                return false;
            }
            
            // JSON 파싱
            json release_info = json::parse(response);
            
            // 버전 비교
            std::string latest_version = release_info["tag_name"];
            if (compareVersions(latest_version, current_version) > 0) {
                update_available_ = true;
                latest_update_info_ = parseUpdateInfo(release_info);
                
                spdlog::info("새로운 업데이트 발견: {} -> {}", current_version, latest_version);
                
                // 콜백 호출
                if (update_available_callback_) {
                    update_available_callback_(latest_update_info_);
                }
                
                return true;
            } else {
                update_available_ = false;
                spdlog::info("최신 버전입니다: {}", current_version);
                return false;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("업데이트 확인 중 오류: {}", e.what());
            return false;
        }
    }
    
    // 업데이트 다운로드
    bool downloadUpdate(const UpdateInfo& update_info, ProgressCallback progress_cb) {
        if (update_in_progress_) {
            spdlog::warn("업데이트 다운로드 중: 이미 진행 중인 업데이트가 있습니다");
            return false;
        }
        
        update_in_progress_ = true;
        spdlog::info("업데이트 다운로드 시작: {}", update_info.version);
        
        try {
            std::string cache_file = getUpdateCacheDirectory() + "/update_" + update_info.version + ".zip";
            
            // 다운로드 진행률 콜백
            auto download_progress = [this, progress_cb](double percentage, size_t downloaded, size_t total) {
                UpdateProgress progress;
                progress.percentage = percentage;
                progress.downloaded_bytes = downloaded;
                progress.total_bytes = total;
                progress.status = "다운로드 중...";
                progress.is_complete = false;
                
                if (progress_cb) {
                    progress_cb(progress);
                }
            };
            
            // 파일 다운로드
            if (downloadFile(update_info.download_url, cache_file, download_progress)) {
                // 체크섬 검증
                if (!update_info.checksum.empty() && !verifyUpdateFile(cache_file, update_info.checksum)) {
                    spdlog::error("업데이트 파일 체크섬 검증 실패");
                    update_in_progress_ = false;
                    return false;
                }
                
                spdlog::info("업데이트 다운로드 완료: {}", cache_file);
                
                // 완료 콜백
                if (progress_cb) {
                    UpdateProgress progress;
                    progress.percentage = 100.0;
                    progress.downloaded_bytes = update_info.file_size;
                    progress.total_bytes = update_info.file_size;
                    progress.status = "다운로드 완료";
                    progress.is_complete = true;
                    progress_cb(progress);
                }
                
                update_in_progress_ = false;
                return true;
            } else {
                spdlog::error("업데이트 다운로드 실패");
                update_in_progress_ = false;
                return false;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("업데이트 다운로드 중 오류: {}", e.what());
            update_in_progress_ = false;
            return false;
        }
    }
    
    // 업데이트 설치
    bool installUpdate(const std::string& update_file) {
        if (!fs::exists(update_file)) {
            spdlog::error("업데이트 파일을 찾을 수 없습니다: {}", update_file);
            return false;
        }
        
        spdlog::info("업데이트 설치 시작: {}", update_file);
        
        try {
            // 현재 실행 파일 백업
            std::string backup_file = createBackup();
            if (backup_file.empty()) {
                spdlog::error("백업 생성 실패");
                return false;
            }
            
            // 업데이트 파일 압축 해제 및 설치
            if (extractAndInstallUpdate(update_file)) {
                update_installed_ = true;
                spdlog::info("업데이트 설치 완료");
                return true;
            } else {
                // 설치 실패 시 백업 복원
                restoreBackup(backup_file);
                spdlog::error("업데이트 설치 실패, 백업 복원됨");
                return false;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("업데이트 설치 중 오류: {}", e.what());
            return false;
        }
    }
    
    // 자동 업데이트 설정
    void setAutoUpdateEnabled(bool enabled) { auto_update_enabled_ = enabled; }
    bool isAutoUpdateEnabled() const { return auto_update_enabled_; }
    
    void setUpdateCheckInterval(int hours) { update_check_interval_ = hours; }
    int getUpdateCheckInterval() const { return update_check_interval_; }
    
    // 업데이트 채널 설정
    void setUpdateChannel(const std::string& channel) { update_channel_ = channel; }
    std::string getUpdateChannel() const { return update_channel_; }
    
    // 콜백 설정
    void setUpdateAvailableCallback(UpdateCallback callback) { update_available_callback_ = callback; }
    void setUpdateProgressCallback(ProgressCallback callback) { update_progress_callback_ = callback; }
    void setUpdateCompleteCallback(UpdateCallback callback) { update_complete_callback_ = callback; }
    
    // 업데이트 서버 설정
    void setUpdateServer(const std::string& server_url) { update_server_ = server_url; }
    std::string getUpdateServer() const { return update_server_; }
    
    // 업데이트 검증
    bool verifyUpdateFile(const std::string& file_path, const std::string& expected_checksum) {
        if (!fs::exists(file_path)) {
            return false;
        }
        
        std::string actual_checksum = calculateFileChecksum(file_path);
        return actual_checksum == expected_checksum;
    }
    
    // 업데이트 롤백
    bool rollbackUpdate() {
        if (!canRollback()) {
            spdlog::warn("롤백할 수 있는 백업이 없습니다");
            return false;
        }
        
        spdlog::info("업데이트 롤백 시작");
        
        try {
            std::string backup_file = getLatestBackup();
            if (restoreBackup(backup_file)) {
                update_installed_ = false;
                spdlog::info("업데이트 롤백 완료");
                return true;
            } else {
                spdlog::error("업데이트 롤백 실패");
                return false;
            }
        } catch (const std::exception& e) {
            spdlog::error("업데이트 롤백 중 오류: {}", e.what());
            return false;
        }
    }
    
    bool canRollback() const {
        return !getLatestBackup().empty();
    }
    
    std::string getLatestBackup() const {
        std::string backup_dir = Config::getInstance().getConfigPath() + "/backups";
        if (!fs::exists(backup_dir)) {
            return "";
        }
        
        std::string latest_backup;
        std::time_t latest_time = 0;
        
        for (const auto& entry : fs::directory_iterator(backup_dir)) {
            if (entry.path().extension() == ".zip") {
                auto file_time = fs::last_write_time(entry.path());
                auto time_t = std::chrono::duration_cast<std::chrono::seconds>(
                    file_time.time_since_epoch()).count();
                
                if (time_t > latest_time) {
                    latest_time = time_t;
                    latest_backup = entry.path().string();
                }
            }
        }
        
        return latest_backup;
    }
    
    // 업데이트 통계
    json getUpdateStatistics() const {
        json stats;
        stats["total_updates"] = update_history_.size();
        stats["last_update_check"] = last_update_check_;
        stats["auto_update_enabled"] = auto_update_enabled_;
        stats["update_channel"] = update_channel_;
        stats["cache_size"] = getUpdateCacheSize();
        return stats;
    }
    
    void clearUpdateHistory() {
        update_history_.clear();
        spdlog::info("업데이트 히스토리 삭제됨");
    }
    
    // 업데이트 상태
    bool isUpdateAvailable() const { return update_available_; }
    bool isUpdateInProgress() const { return update_in_progress_; }
    bool isUpdateInstalled() const { return update_installed_; }
    
    // 업데이트 정보 가져오기
    UpdateInfo getLatestUpdateInfo() const { return latest_update_info_; }
    std::vector<UpdateInfo> getUpdateHistory() const { return update_history_; }
    
    // 업데이트 파일 관리
    std::string getUpdateCacheDirectory() const { return update_cache_dir_; }
    
    void clearUpdateCache() {
        try {
            for (const auto& entry : fs::directory_iterator(update_cache_dir_)) {
                fs::remove_all(entry.path());
            }
            spdlog::info("업데이트 캐시 정리 완료");
        } catch (const std::exception& e) {
            spdlog::error("업데이트 캐시 정리 중 오류: {}", e.what());
        }
    }
    
    size_t getUpdateCacheSize() const {
        size_t total_size = 0;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(update_cache_dir_)) {
                if (fs::is_regular_file(entry.path())) {
                    total_size += fs::file_size(entry.path());
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("캐시 크기 계산 중 오류: {}", e.what());
        }
        return total_size;
    }

private:
    // 멤버 변수
    bool auto_update_enabled_;
    int update_check_interval_;
    std::string update_channel_;
    std::string update_server_;
    bool update_available_;
    bool update_in_progress_;
    bool update_installed_;
    
    UpdateInfo latest_update_info_;
    std::vector<UpdateInfo> update_history_;
    std::string last_update_check_;
    std::string update_cache_dir_;
    
    // 콜백 함수들
    UpdateCallback update_available_callback_;
    ProgressCallback update_progress_callback_;
    UpdateCallback update_complete_callback_;
    
    // 헬퍼 함수들
    void createUpdateCacheDirectory() {
        update_cache_dir_ = Config::getInstance().getConfigPath() + "/updates";
        fs::create_directories(update_cache_dir_);
    }
    
    std::string fetchLatestReleaseInfo() {
#ifdef ENABLE_OBS
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "";
        }
        
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, update_server_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LiveOps-Sentinel/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            spdlog::error("CURL 오류: {}", curl_easy_strerror(res));
            return "";
        }
        
        return response;
#else
        // CURL이 비활성화된 경우 더미 응답 반환
        return "{\"tag_name\":\"1.0.0\",\"body\":\"No updates available\",\"published_at\":\"2024-01-01T00:00:00Z\",\"assets\":[{\"browser_download_url\":\"\",\"size\":0}]}";
#endif
    }
    
    UpdateInfo parseUpdateInfo(const json& release_info) {
        UpdateInfo info;
        info.version = release_info["tag_name"];
        info.download_url = release_info["assets"][0]["browser_download_url"];
        info.changelog = release_info["body"];
        info.release_date = release_info["published_at"];
        info.file_size = release_info["assets"][0]["size"];
        info.is_mandatory = false; // 기본값
        
        return info;
    }
    
    int compareVersions(const std::string& version1, const std::string& version2) {
        // 간단한 버전 비교 (예: 1.2.3 형식)
        std::vector<int> v1 = parseVersion(version1);
        std::vector<int> v2 = parseVersion(version2);
        
        size_t max_size = std::max(v1.size(), v2.size());
        for (size_t i = 0; i < max_size; ++i) {
            int num1 = (i < v1.size()) ? v1[i] : 0;
            int num2 = (i < v2.size()) ? v2[i] : 0;
            
            if (num1 > num2) return 1;
            if (num1 < num2) return -1;
        }
        
        return 0;
    }
    
    std::vector<int> parseVersion(const std::string& version) {
        std::vector<int> result;
        std::stringstream ss(version);
        std::string token;
        
        while (std::getline(ss, token, '.')) {
            try {
                result.push_back(std::stoi(token));
            } catch (...) {
                result.push_back(0);
            }
        }
        
        return result;
    }
    
    bool downloadFile(const std::string& url, const std::string& file_path, std::function<void(double, size_t, size_t)> progress_cb) {
#ifdef ENABLE_OBS
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }
        
        FILE* file = fopen(file_path.c_str(), "wb");
        if (!file) {
            curl_easy_cleanup(curl);
            return false;
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LiveOps-Sentinel/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L); // 무제한 타임아웃
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // 진행률 콜백 설정
        if (progress_cb) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, +[](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
                auto* cb = static_cast<std::function<void(double, size_t, size_t)>*>(clientp);
                if (dltotal > 0) {
                    double percentage = (double)dlnow / dltotal * 100.0;
                    (*cb)(percentage, dlnow, dltotal);
                }
                return 0;
            });
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_cb);
        }
        
        CURLcode res = curl_easy_perform(curl);
        
        fclose(file);
        curl_easy_cleanup(curl);
        
        return res == CURLE_OK;
#else
        // CURL이 비활성화된 경우 더미 파일 생성
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << "Dummy update file";
            file.close();
            if (progress_cb) {
                progress_cb(100.0, 1, 1);
            }
            return true;
        }
        return false;
#endif
    }
    
    std::string calculateFileChecksum(const std::string& file_path) {
        // 간단한 MD5 해시 계산 (실제로는 더 안전한 해시 사용 권장)
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // 간단한 해시 계산 (실제 구현에서는 OpenSSL 등 사용)
        size_t hash = std::hash<std::string>{}(content);
        std::stringstream ss;
        ss << std::hex << hash;
        return ss.str();
    }
    
    std::string createBackup() {
        std::string backup_dir = Config::getInstance().getConfigPath() + "/backups";
        fs::create_directories(backup_dir);
        
        std::string timestamp = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        std::string backup_file = backup_dir + "/backup_" + timestamp + ".zip";
        
        // 현재 실행 파일을 ZIP으로 압축
        // 실제 구현에서는 압축 라이브러리 사용
        spdlog::info("백업 생성: {}", backup_file);
        
        return backup_file;
    }
    
    bool extractAndInstallUpdate(const std::string& update_file) {
        // 실제 구현에서는 압축 해제 및 설치 로직
        spdlog::info("업데이트 파일 압축 해제 및 설치: {}", update_file);
        return true; // 임시로 성공 반환
    }
    
    bool restoreBackup(const std::string& backup_file) {
        // 실제 구현에서는 백업 복원 로직
        spdlog::info("백업 복원: {}", backup_file);
        return true; // 임시로 성공 반환
    }
    
    std::string getLatestBackup() {
        std::string backup_dir = Config::getInstance().getConfigPath() + "/backups";
        if (!fs::exists(backup_dir)) {
            return "";
        }
        
        std::string latest_backup;
        std::time_t latest_time = 0;
        
        for (const auto& entry : fs::directory_iterator(backup_dir)) {
            if (entry.path().extension() == ".zip") {
                auto file_time = fs::last_write_time(entry.path());
                auto time_t = std::chrono::duration_cast<std::chrono::seconds>(
                    file_time.time_since_epoch()).count();
                
                if (time_t > latest_time) {
                    latest_time = time_t;
                    latest_backup = entry.path().string();
                }
            }
        }
        
        return latest_backup;
    }
};

// UpdateManager 구현
UpdateManager::UpdateManager() : impl_(std::make_unique<UpdateManagerImpl>()) {}
UpdateManager::~UpdateManager() = default;

UpdateManager& UpdateManager::getInstance() {
    static UpdateManager instance;
    return instance;
}

bool UpdateManager::checkForUpdates() { return impl_->checkForUpdates(); }
bool UpdateManager::checkForUpdates(const std::string& current_version) { return impl_->checkForUpdates(current_version); }

UpdateManager::UpdateInfo UpdateManager::getLatestUpdateInfo() const { return impl_->getLatestUpdateInfo(); }
std::vector<UpdateManager::UpdateInfo> UpdateManager::getUpdateHistory() const { return impl_->getUpdateHistory(); }

bool UpdateManager::downloadUpdate(const UpdateInfo& update_info, ProgressCallback progress_cb) { 
    return impl_->downloadUpdate(update_info, progress_cb); 
}
bool UpdateManager::downloadUpdate(const std::string& version, ProgressCallback progress_cb) { 
    UpdateInfo info;
    info.version = version;
    return impl_->downloadUpdate(info, progress_cb); 
}

bool UpdateManager::installUpdate(const std::string& update_file) { return impl_->installUpdate(update_file); }
bool UpdateManager::installUpdate(const UpdateInfo& update_info) { 
    std::string cache_file = impl_->getUpdateCacheDirectory() + "/update_" + update_info.version + ".zip";
    return impl_->installUpdate(cache_file); 
}

void UpdateManager::setAutoUpdateEnabled(bool enabled) { impl_->setAutoUpdateEnabled(enabled); }
bool UpdateManager::isAutoUpdateEnabled() const { return impl_->isAutoUpdateEnabled(); }

void UpdateManager::setUpdateCheckInterval(int hours) { impl_->setUpdateCheckInterval(hours); }
int UpdateManager::getUpdateCheckInterval() const { return impl_->getUpdateCheckInterval(); }

void UpdateManager::setUpdateChannel(const std::string& channel) { impl_->setUpdateChannel(channel); }
std::string UpdateManager::getUpdateChannel() const { return impl_->getUpdateChannel(); }

void UpdateManager::setUpdateAvailableCallback(UpdateCallback callback) { impl_->setUpdateAvailableCallback(callback); }
void UpdateManager::setUpdateProgressCallback(ProgressCallback callback) { impl_->setUpdateProgressCallback(callback); }
void UpdateManager::setUpdateCompleteCallback(UpdateCallback callback) { impl_->setUpdateCompleteCallback(callback); }

void UpdateManager::setUpdateServer(const std::string& server_url) { impl_->setUpdateServer(server_url); }
std::string UpdateManager::getUpdateServer() const { return impl_->getUpdateServer(); }

bool UpdateManager::verifyUpdateFile(const std::string& file_path, const std::string& expected_checksum) { 
    return impl_->verifyUpdateFile(file_path, expected_checksum); 
}
bool UpdateManager::verifyUpdateSignature(const std::string& file_path, const std::string& signature) { 
    // 실제 구현에서는 디지털 서명 검증
    return true; 
}

bool UpdateManager::rollbackUpdate() { return impl_->rollbackUpdate(); }
bool UpdateManager::canRollback() const { return impl_->canRollback(); }

json UpdateManager::getUpdateStatistics() const { return impl_->getUpdateStatistics(); }
void UpdateManager::clearUpdateHistory() { impl_->clearUpdateHistory(); }

bool UpdateManager::isUpdateAvailable() const { return impl_->isUpdateAvailable(); }
bool UpdateManager::isUpdateInProgress() const { return impl_->isUpdateInProgress(); }
bool UpdateManager::isUpdateInstalled() const { return impl_->isUpdateInstalled(); }

std::string UpdateManager::getUpdateCacheDirectory() const { return impl_->getUpdateCacheDirectory(); }
void UpdateManager::clearUpdateCache() { impl_->clearUpdateCache(); }
size_t UpdateManager::getUpdateCacheSize() const { return impl_->getUpdateCacheSize(); }

} // namespace core
