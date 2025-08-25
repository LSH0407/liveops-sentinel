#pragma once
#include <cstdint>

namespace diag {

enum class EncoderType { Auto=0, X264=1, NVENC=2, HEVC_NVENC=3 };
enum class PresetType  { UltraFast, VeryFast, Fast, Quality, Performance };

struct ObsMetrics {
  int outputWidth{1920};
  int outputHeight{1080};
  int fps{60};
  double encodingLagMs{0.0};
  double renderLagMs{0.0};
};

struct Recommendation {
  EncoderType encoder{EncoderType::Auto};
  PresetType  preset{PresetType::Quality};
  int minKbps{2500};
  int maxKbps{6000};
};

class RecommendationEngine {
public:
  EncoderType   selectEncoder(int preferred /*0:auto,1:x264,2:nvenc,3:hevc*/);
  PresetType    selectPreset(const ObsMetrics& m, EncoderType encoder);
  Recommendation recommend(const ObsMetrics& m,
                           int preferredEncoder,
                           int networkKbps,
                           double headroom = 0.8);
};

} // namespace diag