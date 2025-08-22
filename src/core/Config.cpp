#include "Config.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <filesystem>

bool LoadConfig(const std::string& path, Config& out){
  std::ifstream f(path); if(!f.good()) return false;
  nlohmann::json j; f >> j; out = j.get<Config>(); return true;
}
bool SaveConfig(const std::string& path, const Config& in){
  std::ofstream f(path); if(!f.good()) return false;
  nlohmann::json j = in; f << j.dump(2); return true;
}

static std::filesystem::path GetWindowsConfigPath(){
#ifdef _WIN32
  const char* appdata = std::getenv("APPDATA");
  std::filesystem::path base = appdata? std::filesystem::path(appdata) : std::filesystem::path(".");
  return base / "LiveOpsSentinel" / "config.json";
#else
  return {};
#endif
}

static std::filesystem::path GetLinuxConfigPath(){
#ifndef _WIN32
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg){
    return std::filesystem::path(xdg) / "liveops-sentinel" / "config.json";
  }
  const char* home = std::getenv("HOME");
  std::filesystem::path base = home? std::filesystem::path(home) : std::filesystem::path(".");
  return base / ".config" / "liveops-sentinel" / "config.json";
#else
  return {};
#endif
}

std::filesystem::path GetUserConfigPath(){
#ifdef _WIN32
  return GetWindowsConfigPath();
#else
  return GetLinuxConfigPath();
#endif
}

bool LoadUserConfig(Config& out){
  auto path = GetUserConfigPath();
  std::ifstream f(path);
  if(!f.good()){
    // 최초 생성 시 기본값으로 저장해 둘 수 있음
    return false;
  }
  try{
    nlohmann::json j; f >> j; out = j.get<Config>();
    return true;
  }catch(const std::exception& e){
    spdlog::error("LoadUserConfig failed: {}", e.what());
    return false;
  }
}

bool SaveUserConfig(const Config& in){
  try{
    auto path = GetUserConfigPath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if(!f.good()) return false;
    nlohmann::json j = in; f << j.dump(2);
    return true;
  }catch(const std::exception& e){
    spdlog::error("SaveUserConfig failed: {}", e.what());
    return false;
  }
}
