#pragma once
#include <string>
#include <vector>

enum class EncoderType {
    NVENC,
    X264,
    QSV,
    AMD
};

enum class PresetType {
    QUALITY,
    PERFORMANCE,
    ULTRAFAST,
    VERYFAST,
    FAST,
    MEDIUM,
    SLOW,
    SLOWER,
    VERYSLOW
};

struct ObsMetrics {
    double droppedFramesRatio{0.0};
    double outputSkippedFrames{0.0};
    double averageFrameRenderTimeMs{0.0};
    double cpuUsage{0.0};
    double memoryUsageMB{0.0};
    double activeFps{0.0};
    int64_t outputBytes{0};
    double strain{0.0};
    double encodingLagMs{0.0};
    double renderLagMs{0.0};
};

struct SystemMetrics {
    double cpuPct{0.0};
    double gpuPct{0.0};
    double diskWriteMBps{0.0};
    double memoryMB{0.0};
};

struct NetworkMetrics {
    double sustainedUplinkMbps{0.0};
    double rttMs{0.0};
    double lossPct{0.0};
    double jitterMs{0.0};
};

struct VideoSettings {
    int baseWidth{1920};
    int baseHeight{1080};
    int outputWidth{1920};
    int outputHeight{1080};
    double fps{60.0};
};

struct ObsRecommendation {
    EncoderType encoder{EncoderType::NVENC};
    int bitrateKbps{0};
    int keyframeSec{2};
    int vbvBufferKbps{0};
    PresetType preset{PresetType::QUALITY};
    std::string profile{"main"};
    std::string notes;
    
    // 추가 설정
    int width{1920};
    int height{1080};
    int fps{60};
    std::string rateControl{"CBR"};
    int maxBitrateKbps{0};
    double qualityScale{0.0};
};

struct RecommendationInput {
    ObsMetrics obs;
    SystemMetrics system;
    NetworkMetrics network;
    VideoSettings video;
    EncoderType preferredEncoder{EncoderType::NVENC};
    double headroom{0.75};
    int minKbps{800};
    int maxKbps{15000};
};

class RecommendationEngine {
public:
    static ObsRecommendation RecommendObsSettings(const RecommendationInput& input);
    
private:
    static double calculateSafeBitrate(const NetworkMetrics& network, double headroom);
    static PresetType selectPreset(const ObsMetrics& obs, const SystemMetrics& system, EncoderType encoder);
    static EncoderType selectEncoder(const SystemMetrics& system, EncoderType preferred);
    static int calculateKeyframeInterval(const NetworkMetrics& network);
    static int calculateVbvBuffer(const ObsRecommendation& rec, const NetworkMetrics& network);
    static std::string generateNotes(const RecommendationInput& input, const ObsRecommendation& rec);
    static int clampBitrateByResolution(int bitrate, const VideoSettings& video, int minKbps, int maxKbps);
};
