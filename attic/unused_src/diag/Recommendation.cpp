#include "Recommendation.h"
#include <algorithm>
#include <numeric>

using namespace diag;

static int clampInt(int v, int lo, int hi) {
  return std::max(lo, std::min(hi, v));
}

EncoderType RecommendationEngine::selectEncoder(int preferred) {
  switch (preferred) {
    case 1: return EncoderType::X264;
    case 2: return EncoderType::NVENC;
    case 3: return EncoderType::HEVC_NVENC;
    default: return EncoderType::Auto;
  }
}

PresetType RecommendationEngine::selectPreset(const ObsMetrics& m, EncoderType encoder) {
  // 간단 규칙: 렌더/인코딩 랙이 크면 가벼운 프리셋
  if (m.encodingLagMs > 25.0 || m.renderLagMs > 20.0) {
    return (encoder == EncoderType::X264) ? PresetType::UltraFast : PresetType::Performance;
  }
  return (encoder == EncoderType::X264) ? PresetType::VeryFast : PresetType::Quality;
}

Recommendation RecommendationEngine::recommend(const ObsMetrics& m,
                                               int preferredEncoder,
                                               int networkKbps,
                                               double headroom) {
  Recommendation r{};
  r.encoder = selectEncoder(preferredEncoder);
  r.preset  = selectPreset(m, r.encoder);

  // 해상도/프레임 기반 목표 비트레이트(bpp 추정)
  double bpp = 0.085;                 // NVENC 기본
  if (r.encoder == EncoderType::X264)      bpp = 0.10;
  else if (r.encoder == EncoderType::HEVC_NVENC) bpp = 0.06;

  double targetKbps = (m.outputWidth * 1.0 * m.outputHeight * m.fps * bpp) / 1000.0;
  int safeUp = static_cast<int>(networkKbps * headroom);

  int minK = static_cast<int>(targetKbps * 0.7);
  int maxK = static_cast<int>(targetKbps * 1.2);
  maxK = std::min(maxK, safeUp);
  minK = std::min(minK, maxK - 250);

  // 합리적 클램프
  minK = clampInt(minK, 600, 20000);
  maxK = clampInt(maxK, 1000, 25000);

  r.minKbps = minK;
  r.maxKbps = std::max(minK + 250, maxK);
  return r;
}