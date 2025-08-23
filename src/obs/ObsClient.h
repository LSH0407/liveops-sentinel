#pragma once
#include <string>
#include <vector>
#include <optional>

namespace obs {
struct Stats { 
    double droppedFramesRatio{0.0}; 
    double avgRenderMs{0.0}; 
    double cpuPct{0.0}; 
};

struct VideoSettings { 
    int baseWidth{1920}, baseHeight{1080}, outputWidth{1920}, outputHeight{1080}, fps{60}; 
};

class ObsClient {
public:
    bool connect(const std::string& host, int port, const std::string& password);
    bool isConnected() const;
    bool startStream(); 
    bool stopStream();
    bool startRecord(); 
    bool stopRecord();
    bool setCurrentProgramScene(const std::string& name);
    bool getSceneList(std::vector<std::string>& out);
    std::optional<Stats> getStats();
    std::optional<VideoSettings> getVideoSettings();
};
} // namespace obs
