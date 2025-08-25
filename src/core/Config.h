#pragma once
#include <string>
#include <map>
#include <vector>

namespace core {

class Config {
public:
    static Config& getInstance();
    
    Config();
    ~Config();
    
    // 설정 로드/저장
    void loadConfig();
    void saveConfig();
    void createDefaultConfig();
    std::string getConfigPath();
    
    // 네트워크 설정
    std::string getProbeHost() const;
    void setProbeHost(const std::string& host);
    int getProbeIntervalMs() const;
    void setProbeIntervalMs(int interval_ms);
    
    // UI 설정
    std::string getTheme() const;
    void setTheme(const std::string& theme);
    bool getSimpleMode() const;
    void setSimpleMode(bool simple_mode);
    
    // 플랫폼 설정
    std::string getPlatform() const;
    void setPlatform(const std::string& platform);
    int getDiagnosticMinutes() const;
    void setDiagnosticMinutes(int minutes);
    
    // 웹훅 설정
    std::string getWebhookUrl() const;
    void setWebhookUrl(const std::string& url);
    
    // 로깅 설정
    std::string getLogLevel() const;
    void setLogLevel(const std::string& level);
    bool getLogFileEnabled() const;
    void setLogFileEnabled(bool enabled);
    bool getLogConsoleEnabled() const;
    void setLogConsoleEnabled(bool enabled);
    
    // 전체 설정 데이터 접근
    std::map<std::string, std::string> getConfigData() const;
    void setConfigData(const std::map<std::string, std::string>& data);
    
private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    std::map<std::string, std::string> config_data_;
};

} // namespace core
