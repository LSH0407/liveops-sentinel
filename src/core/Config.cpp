#include "Config.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace core {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config() {
    loadConfig();
}

Config::~Config() {
    saveConfig();
}

void Config::loadConfig() {
    std::string config_path = getConfigPath();
    
    // 설정 디렉토리 생성
    fs::path config_dir = fs::path(config_path).parent_path();
    if (!fs::exists(config_dir)) {
        fs::create_directories(config_dir);
    }
    
    // 설정 파일이 없으면 기본 설정 생성
    if (!fs::exists(config_path)) {
        createDefaultConfig();
        return;
    }
    
    try {
        std::ifstream file(config_path);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    config_data_[key] = value;
                }
            }
            file.close();
        } else {
            createDefaultConfig();
        }
    } catch (const std::exception& e) {
        std::cerr << "설정 파일 로드 오류: " << e.what() << std::endl;
        createDefaultConfig();
    }
}

void Config::saveConfig() {
    try {
        std::string config_path = getConfigPath();
        std::ofstream file(config_path);
        if (file.is_open()) {
            for (const auto& pair : config_data_) {
                file << pair.first << "=" << pair.second << std::endl;
            }
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "설정 파일 저장 오류: " << e.what() << std::endl;
    }
}

void Config::createDefaultConfig() {
    config_data_ = {
        {"net.probe_host", "8.8.8.8"},
        {"net.interval_ms", "1000"},
        {"ui.theme", "dark"},
        {"ui.simpleMode", "true"},
        {"platform", "soop"},
        {"diag_minutes", "60"},
        {"webhook", ""},
        {"logging.level", "info"},
        {"logging.file_enabled", "true"},
        {"logging.console_enabled", "true"}
    };
    saveConfig();
}

std::string Config::getConfigPath() {
#ifdef _WIN32
    std::string appdata = std::getenv("APPDATA") ? std::getenv("APPDATA") : "";
    if (appdata.empty()) {
        appdata = "C:\\Users\\" + std::string(std::getenv("USERNAME") ? std::getenv("USERNAME") : "User") + "\\AppData\\Roaming";
    }
    return appdata + "\\LiveOpsSentinel\\config.txt";
#else
    std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
    return home + "/.config/liveops-sentinel/config.txt";
#endif
}

// 네트워크 설정
std::string Config::getProbeHost() const {
    auto it = config_data_.find("net.probe_host");
    return it != config_data_.end() ? it->second : "8.8.8.8";
}

void Config::setProbeHost(const std::string& host) {
    config_data_["net.probe_host"] = host;
}

int Config::getProbeIntervalMs() const {
    auto it = config_data_.find("net.interval_ms");
    return it != config_data_.end() ? std::stoi(it->second) : 1000;
}

void Config::setProbeIntervalMs(int interval_ms) {
    config_data_["net.interval_ms"] = std::to_string(interval_ms);
}

// UI 설정
std::string Config::getTheme() const {
    auto it = config_data_.find("ui.theme");
    return it != config_data_.end() ? it->second : "dark";
}

void Config::setTheme(const std::string& theme) {
    config_data_["ui.theme"] = theme;
}

bool Config::getSimpleMode() const {
    auto it = config_data_.find("ui.simpleMode");
    return it != config_data_.end() ? (it->second == "true") : true;
}

void Config::setSimpleMode(bool simple_mode) {
    config_data_["ui.simpleMode"] = simple_mode ? "true" : "false";
}

// 플랫폼 설정
std::string Config::getPlatform() const {
    auto it = config_data_.find("platform");
    return it != config_data_.end() ? it->second : "soop";
}

void Config::setPlatform(const std::string& platform) {
    config_data_["platform"] = platform;
}

int Config::getDiagnosticMinutes() const {
    auto it = config_data_.find("diag_minutes");
    return it != config_data_.end() ? std::stoi(it->second) : 60;
}

void Config::setDiagnosticMinutes(int minutes) {
    config_data_["diag_minutes"] = std::to_string(minutes);
}

// 웹훅 설정
std::string Config::getWebhookUrl() const {
    auto it = config_data_.find("webhook");
    return it != config_data_.end() ? it->second : "";
}

void Config::setWebhookUrl(const std::string& url) {
    config_data_["webhook"] = url;
}

// 로깅 설정
std::string Config::getLogLevel() const {
    auto it = config_data_.find("logging.level");
    return it != config_data_.end() ? it->second : "info";
}

void Config::setLogLevel(const std::string& level) {
    config_data_["logging.level"] = level;
}

bool Config::getLogFileEnabled() const {
    auto it = config_data_.find("logging.file_enabled");
    return it != config_data_.end() ? (it->second == "true") : true;
}

void Config::setLogFileEnabled(bool enabled) {
    config_data_["logging.file_enabled"] = enabled ? "true" : "false";
}

bool Config::getLogConsoleEnabled() const {
    auto it = config_data_.find("logging.console_enabled");
    return it != config_data_.end() ? (it->second == "true") : true;
}

void Config::setLogConsoleEnabled(bool enabled) {
    config_data_["logging.console_enabled"] = enabled ? "true" : "false";
}

// 전체 설정 데이터 접근
std::map<std::string, std::string> Config::getConfigData() const {
    return config_data_;
}

void Config::setConfigData(const std::map<std::string, std::string>& data) {
    config_data_ = data;
}

} // namespace core
