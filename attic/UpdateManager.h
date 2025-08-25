#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace core {

struct UpdateInfo {
    std::string version;
    std::string download_url;
    std::string changelog;
    std::string release_date;
    std::string checksum;
    size_t file_size;
    bool is_mandatory;
    std::vector<std::string> dependencies;
};

struct UpdateProgress {
    double percentage;
    size_t downloaded_bytes;
    size_t total_bytes;
    std::string status;
    bool is_complete;
    std::string error_message;
};

class UpdateManager {
public:
    using ProgressCallback = std::function<void(const UpdateProgress&)>;
    using UpdateCallback = std::function<void(const UpdateInfo&)>;

    UpdateManager();
    ~UpdateManager();

    // 싱글톤 인스턴스
    static UpdateManager& getInstance();

    // 업데이트 확인
    bool checkForUpdates();
    bool checkForUpdates(const std::string& current_version);
    
    // 업데이트 정보 가져오기
    UpdateInfo getLatestUpdateInfo() const;
    std::vector<UpdateInfo> getUpdateHistory() const;
    
    // 업데이트 다운로드
    bool downloadUpdate(const UpdateInfo& update_info, ProgressCallback progress_cb = nullptr);
    bool downloadUpdate(const std::string& version, ProgressCallback progress_cb = nullptr);
    
    // 업데이트 설치
    bool installUpdate(const std::string& update_file);
    bool installUpdate(const UpdateInfo& update_info);
    
    // 자동 업데이트 설정
    void setAutoUpdateEnabled(bool enabled);
    bool isAutoUpdateEnabled() const;
    
    void setUpdateCheckInterval(int hours);
    int getUpdateCheckInterval() const;
    
    // 업데이트 채널 설정
    void setUpdateChannel(const std::string& channel);
    std::string getUpdateChannel() const;
    
    // 콜백 설정
    void setUpdateAvailableCallback(UpdateCallback callback);
    void setUpdateProgressCallback(ProgressCallback callback);
    void setUpdateCompleteCallback(UpdateCallback callback);
    
    // 업데이트 서버 설정
    void setUpdateServer(const std::string& server_url);
    std::string getUpdateServer() const;
    
    // 업데이트 검증
    bool verifyUpdateFile(const std::string& file_path, const std::string& expected_checksum);
    bool verifyUpdateSignature(const std::string& file_path, const std::string& signature);
    
    // 업데이트 롤백
    bool rollbackUpdate();
    bool canRollback() const;
    
    // 업데이트 통계
    json getUpdateStatistics() const;
    void clearUpdateHistory();
    
    // 업데이트 상태
    bool isUpdateAvailable() const;
    bool isUpdateInProgress() const;
    bool isUpdateInstalled() const;
    
    // 업데이트 파일 관리
    std::string getUpdateCacheDirectory() const;
    void clearUpdateCache();
    size_t getUpdateCacheSize() const;

private:
    class UpdateManagerImpl;
    std::unique_ptr<UpdateManagerImpl> impl_;
};

} // namespace core
