"""
플랫폼별 스트리밍 규칙 및 권장 설정
"""

from typing import Dict, List, Tuple
from dataclasses import dataclass

@dataclass
class PlatformRule:
    """플랫폼별 스트리밍 규칙"""
    name: str
    max_video_bitrate_kbps: int
    max_audio_bitrate_kbps: int
    keyframe_interval_sec: int
    max_fps: int
    recommended_resolutions: List[Tuple[str, int, int, int]]  # (name, width, height, max_bitrate)
    encoder_hints: List[str]
    notes: str

# 플랫폼별 규칙 정의
PLATFORM_RULES = {
    "soop": PlatformRule(
        name="SOOP (숲)",
        max_video_bitrate_kbps=8000,
        max_audio_bitrate_kbps=128,
        keyframe_interval_sec=1,
        max_fps=60,
        recommended_resolutions=[
            ("1080p60", 1920, 1080, 8000),
            ("1080p30", 1920, 1080, 6000),
            ("720p60", 1280, 720, 5000),
            ("720p30", 1280, 720, 3500),
            ("480p30", 854, 480, 2000),
        ],
        encoder_hints=["NVENC", "QSV", "x264"],
        notes="한국 스트리밍 플랫폼. 안정적인 연결이 중요합니다."
    ),
    
    "chzzk": PlatformRule(
        name="CHZZK (치지직)",
        max_video_bitrate_kbps=8000,
        max_audio_bitrate_kbps=128,
        keyframe_interval_sec=1,
        max_fps=60,
        recommended_resolutions=[
            ("1080p60", 1920, 1080, 8000),
            ("1080p30", 1920, 1080, 6000),
            ("720p60", 1280, 720, 5000),
            ("720p30", 1280, 720, 3500),
            ("480p30", 854, 480, 2000),
        ],
        encoder_hints=["NVENC", "QSV", "x264"],
        notes="네이버 스트리밍 플랫폼. HW 인코더 권장."
    ),
    
    "youtube": PlatformRule(
        name="YouTube",
        max_video_bitrate_kbps=15000,
        max_audio_bitrate_kbps=128,
        keyframe_interval_sec=2,
        max_fps=60,
        recommended_resolutions=[
            ("1080p60", 1920, 1080, 12000),
            ("1080p30", 1920, 1080, 8000),
            ("720p60", 1280, 720, 9000),
            ("720p30", 1280, 720, 5000),
            ("480p30", 854, 480, 2500),
        ],
        encoder_hints=["NVENC", "QSV", "x264"],
        notes="글로벌 플랫폼. CBR 모드 권장, 키프레임 2초."
    ),
    
    "twitch": PlatformRule(
        name="Twitch",
        max_video_bitrate_kbps=6000,
        max_audio_bitrate_kbps=160,
        keyframe_interval_sec=2,
        max_fps=60,
        recommended_resolutions=[
            ("1080p60", 1920, 1080, 6000),
            ("1080p30", 1920, 1080, 4500),
            ("720p60", 1280, 720, 4500),
            ("720p30", 1280, 720, 3000),
            ("480p30", 854, 480, 1500),
        ],
        encoder_hints=["NVENC", "QSV", "x264"],
        notes="글로벌 플랫폼. 안정적인 연결이 중요합니다."
    ),
    

}

def get_platform_rule(platform: str) -> PlatformRule:
    """플랫폼 규칙 반환"""
    return PLATFORM_RULES.get(platform.lower(), PLATFORM_RULES["soop"])

def get_recommended_settings(platform: str, uplink_kbps: float, rtt_ms: float, loss_pct: float) -> Dict:
    """네트워크 상태에 따른 권장 설정 반환"""
    rule = get_platform_rule(platform)
    
    # 안전한 비트레이트 계산 (업로드의 70% 사용)
    safe_bitrate_kbps = min(uplink_kbps * 0.7, rule.max_video_bitrate_kbps)
    
    # 네트워크 상태에 따른 해상도 선택
    if loss_pct > 2.0 or rtt_ms > 100:
        # 네트워크 상태 나쁨 - 낮은 해상도
        resolution = rule.recommended_resolutions[-1]  # 가장 낮은 해상도
    elif loss_pct > 0.5 or rtt_ms > 60:
        # 네트워크 상태 보통 - 중간 해상도
        resolution = rule.recommended_resolutions[2]  # 720p30
    else:
        # 네트워크 상태 좋음 - 높은 해상도
        resolution = rule.recommended_resolutions[0]  # 1080p60
    
    # 최종 비트레이트 결정
    final_bitrate = min(safe_bitrate_kbps, resolution[3])
    
    return {
        "platform": rule.name,
        "resolution": f"{resolution[1]}x{resolution[2]}",
        "fps": 60 if "60" in resolution[0] else 30,
        "video_bitrate_kbps": int(final_bitrate),
        "audio_bitrate_kbps": rule.max_audio_bitrate_kbps,
        "keyframe_interval_sec": rule.keyframe_interval_sec,
        "encoder_hint": rule.encoder_hints[0],
        "rationale": f"업로드 {uplink_kbps:.1f}Mbps, 응답시간 {rtt_ms:.1f}ms, 손실 {loss_pct:.1f}% → {resolution[0]} {int(final_bitrate)}kbps 권장"
    }

def get_platform_list() -> List[str]:
    """지원하는 플랫폼 목록 반환"""
    return list(PLATFORM_RULES.keys())

def get_platform_display_names() -> Dict[str, str]:
    """플랫폼 표시명 반환"""
    return {k: v.name for k, v in PLATFORM_RULES.items()}
