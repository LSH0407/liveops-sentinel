#include "Recommendation.h"
#include <algorithm>
#include <cmath>
#include <sstream>

ObsRecommendation RecommendationEngine::RecommendObsSettings(const RecommendationInput& input) {
    ObsRecommendation rec;
    
    // 기본 해상도/프레임레이트 설정
    rec.width = input.video.outputWidth;
    rec.height = input.video.outputHeight;
    rec.fps = static_cast<int>(input.video.fps);
    
    // 인코더 선택 (선호도 고려)
    rec.encoder = selectEncoder(input.system, input.preferredEncoder);
    
    // 안전한 비트레이트 계산
    double safeBitrate = calculateSafeBitrate(input.network, input.headroom);
    
    // 네트워크 불안정성에 따른 추가 헤드룸 적용
    if (input.network.rttMs > 80.0 || input.network.lossPct > 1.0) {
        safeBitrate *= 0.6; // 40% 추가 축소 (더 보수적)
    } else if (input.network.rttMs > 50.0 || input.network.lossPct > 0.5) {
        safeBitrate *= 0.8; // 20% 추가 축소
    }
    
    // 해상도/프레임레이트별 비트레이트 클램핑
    int rawBitrate = static_cast<int>(std::round(safeBitrate * 1000.0));
    rec.bitrateKbps = clampBitrateByResolution(rawBitrate, input.video, input.minKbps, input.maxKbps);
    
    // 프리셋 선택
    rec.preset = selectPreset(input.obs, input.system, rec.encoder);
    
    // 키프레임 간격 계산
    rec.keyframeSec = calculateKeyframeInterval(input.network);
    
    // VBV 버퍼 크기 계산
    rec.vbvBufferKbps = calculateVbvBuffer(rec, input.network);
    
    // 프로파일 설정
    if (rec.encoder == EncoderType::NVENC) {
        rec.profile = "main";
        if (rec.width >= 3840 || rec.height >= 2160) {
            rec.profile = "main10";
        }
    } else if (rec.encoder == EncoderType::X264) {
        rec.profile = "high";
        if (rec.width >= 3840 || rec.height >= 2160) {
            rec.profile = "high10";
        }
    }
    
    // 품질 스케일 (VBR 모드용)
    if (input.network.lossPct < 0.5 && input.network.rttMs < 50.0) {
        rec.qualityScale = 0.8; // 고품질
    } else {
        rec.qualityScale = 0.6; // 안정성 우선
    }
    
    // 노트 생성
    rec.notes = generateNotes(input, rec);
    
    return rec;
}

double RecommendationEngine::calculateSafeBitrate(const NetworkMetrics& network, double headroom) {
    double safeNet = network.sustainedUplinkMbps * headroom;
    
    // 네트워크 품질에 따른 추가 조정
    if (network.lossPct > 2.0) {
        safeNet *= 0.7; // 높은 손실률
    } else if (network.lossPct > 1.0) {
        safeNet *= 0.85; // 중간 손실률
    }
    
    if (network.rttMs > 100.0) {
        safeNet *= 0.8; // 높은 지연시간
    } else if (network.rttMs > 50.0) {
        safeNet *= 0.9; // 중간 지연시간
    }
    
    return safeNet;
}

PresetType RecommendationEngine::selectPreset(const ObsMetrics& obs, const SystemMetrics& system, EncoderType encoder) {
    if (encoder == EncoderType::NVENC) {
        // NVENC: GPU 사용률 기반
        if (system.gpuPct > 85.0 || obs.encodingLagMs > 25.0) {
            return PresetType::PERFORMANCE;
        } else {
            return PresetType::QUALITY;
        }
    } else if (encoder == EncoderType::X264) {
        // x264: CPU 사용률 기반
        if (system.cpuPct > 85.0 || obs.encodingLagMs > 25.0) {
            return PresetType::ULTRAFAST;
        } else if (system.cpuPct > 70.0) {
            return PresetType::VERYFAST;
        } else if (system.cpuPct > 50.0) {
            return PresetType::FAST;
        } else {
            return PresetType::MEDIUM;
        }
    }
    
    return PresetType::QUALITY;
}

EncoderType RecommendationEngine::selectEncoder(const SystemMetrics& system, EncoderType preferred) {
    // 선호 인코더가 사용 가능한지 확인
    if (preferred == EncoderType::NVENC && system.gpuPct < 85.0) {
        return EncoderType::NVENC;
    }
    
    if (preferred == EncoderType::X264 && system.cpuPct < 85.0) {
        return EncoderType::X264;
    }
    
    // 선호 인코더가 부적절하면 대안 선택
    if (system.gpuPct < 70.0 && system.cpuPct < 80.0) {
        return EncoderType::NVENC;
    }
    
    if (system.cpuPct < 70.0) {
        return EncoderType::X264;
    }
    
    // 기본값
    return EncoderType::NVENC;
}

int RecommendationEngine::calculateKeyframeInterval(const NetworkMetrics& network) {
    // 네트워크 불안정 시 짧은 키프레임 간격
    if (network.lossPct > 2.0 || network.rttMs > 100.0) {
        return 1;
    } else if (network.lossPct > 1.0 || network.rttMs > 50.0) {
        return 2;
    } else {
        return 2; // 기본값
    }
}

int RecommendationEngine::calculateVbvBuffer(const ObsRecommendation& rec, const NetworkMetrics& network) {
    int baseBuffer = rec.bitrateKbps;
    
    // 네트워크 불안정 시 버퍼 크기 축소
    if (network.lossPct > 2.0 || network.rttMs > 100.0) {
        return static_cast<int>(baseBuffer * 0.5);
    } else if (network.lossPct > 1.0 || network.rttMs > 50.0) {
        return static_cast<int>(baseBuffer * 0.8);
    }
    
    return baseBuffer;
}

std::string RecommendationEngine::generateNotes(const RecommendationInput& input, const ObsRecommendation& rec) {
    std::ostringstream notes;
    
    notes << "headroom " << input.headroom << "; ";
    
    if (input.network.lossPct > 0.0) {
        notes << "loss " << input.network.lossPct << "% → ";
        if (input.network.lossPct < 1.0) {
            notes << "유지 가능";
        } else if (input.network.lossPct < 2.0) {
            notes << "주의 필요";
        } else {
            notes << "품질 저하 가능";
        }
    } else {
        notes << "안정적인 네트워크";
    }
    
    if (input.obs.encodingLagMs > 25.0) {
        notes << "; 인코딩 지연 높음";
    }
    
    if (input.obs.droppedFramesRatio > 0.02) {
        notes << "; 프레임 드롭 발생";
    }
    
    return notes.str();
}

int RecommendationEngine::clampBitrateByResolution(int bitrate, const VideoSettings& video, int minKbps, int maxKbps) {
    // 해상도/프레임레이트별 권장 비트레이트 범위
    int pixels = video.outputWidth * video.outputHeight;
    double fps = video.fps;
    
    int recommendedMin = minKbps;
    int recommendedMax = maxKbps;
    
    // 해상도별 기본 범위
    if (pixels <= 1280 * 720) { // 720p
        if (fps <= 30) {
            recommendedMin = std::max(minKbps, 2500);
            recommendedMax = std::min(maxKbps, 4500);
        } else { // 60fps
            recommendedMin = std::max(minKbps, 3500);
            recommendedMax = std::min(maxKbps, 6000);
        }
    } else if (pixels <= 1920 * 1080) { // 1080p
        if (fps <= 30) {
            recommendedMin = std::max(minKbps, 4500);
            recommendedMax = std::min(maxKbps, 7000);
        } else { // 60fps
            recommendedMin = std::max(minKbps, 6000);
            recommendedMax = std::min(maxKbps, 9000);
        }
    } else if (pixels <= 2560 * 1440) { // 1440p
        if (fps <= 30) {
            recommendedMin = std::max(minKbps, 7000);
            recommendedMax = std::min(maxKbps, 11000);
        } else { // 60fps
            recommendedMin = std::max(minKbps, 9000);
            recommendedMax = std::min(maxKbps, 14000);
        }
    } else { // 4K
        if (fps <= 30) {
            recommendedMin = std::max(minKbps, 13000);
            recommendedMax = std::min(maxKbps, 20000);
        } else { // 60fps
            recommendedMin = std::max(minKbps, 18000);
            recommendedMax = std::min(maxKbps, 25000);
        }
    }
    
    // 최종 클램핑
    return std::clamp(bitrate, recommendedMin, recommendedMax);
}
