# LiveOps Sentinel

**기본 모니터링 에디션 - 실시간 스트리밍 환경 진단 도구**

[![Build Status](https://github.com/your-username/liveops-sentinel/workflows/Build/badge.svg)](https://github.com/your-username/liveops-sentinel/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-green.svg)](https://github.com/your-username/liveops-sentinel/releases)

## 📋 개요

LiveOps Sentinel은 스트리머를 위한 실시간 모니터링 및 진단 도구입니다. 네트워크 상태, 시스템 성능을 실시간으로 추적하고, 플랫폼별 최적 설정을 권장하여 안정적인 스트리밍 환경을 제공합니다.

## ✨ 주요 기능

### 🔍 실시간 모니터링
- **네트워크 메트릭**: 응답 시간(RTT), 패킷 손실률, 업로드 대역폭
- **시스템 메트릭**: CPU, 메모리 사용률
- **사용자 친화적 UI**: 직관적인 용어와 시각적 표시

### 🎯 플랫폼별 최적화
- **지원 플랫폼**: SOOP(숲), CHZZK(치지직), YouTube, Twitch, 아프리카TV
- **자동 권장 설정**: 네트워크 상태에 따른 해상도/비트레이트/FPS 추천
- **상한 경고**: 플랫폼별 제한 초과 시 자동 하향 조정 제안

### 📊 진단 모드
- **지정 시간 진단**: 15/30/60/120분 또는 사용자 지정
- **실시간 진행 표시**: 진행바와 남은 시간 표시
- **상세 리포트**: HTML, JSON, CSV 형식으로 결과 저장

### 🎨 직관적 인터페이스
- **다크 테마**: 눈의 피로도 감소
- **간단/전문 모드**: 사용자 수준에 맞는 정보 표시
- **실시간 그래프**: 60초 롤링 윈도우로 트렌드 파악

## 🏗️ 아키텍처

```
liveops-sentinel/
├── src/                    # C++ 백엔드
│   ├── main.cpp           # 메인 실행 파일
│   ├── core/              # 핵심 모듈
│   │   ├── Config.cpp     # 설정 관리
│   │   └── SystemMetrics.cpp  # 시스템 메트릭
│   └── net/               # 네트워크 모듈
│       └── Probe.cpp      # 네트워크 진단
├── ui_py/                 # Python GUI
│   ├── main.py           # GUI 메인
│   ├── views/            # 뷰 컴포넌트
│   │   └── dashboard.py  # 메인 대시보드
│   ├── widgets/          # UI 위젯
│   ├── core/             # 핵심 로직
│   ├── actions/          # 액션 모듈
│   │   └── diagnose.py   # 진단 모드
│   └── platform_rules.py # 플랫폼 규칙
└── scripts/              # 빌드 스크립트
```

## 🚀 빠른 시작

### 요구사항

- **Windows 10/11** (x64)
- **Visual Studio 2022** 또는 **Build Tools**
- **Python 3.8+** (GUI용)
- **CMake 3.22+**

### 설치 및 실행

1. **저장소 클론**
```bash
git clone https://github.com/your-username/liveops-sentinel.git
cd liveops-sentinel
```

2. **C++ 백엔드 빌드**
```bash
# vcpkg 설정 (필요시)
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# 빌드
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

3. **Python GUI 실행**
```bash
cd ui_py
python -m pip install -r requirements.txt
python main.py
```

### 첫 실행 설정

1. **백엔드 실행 파일 경로**: 자동으로 찾거나 수동 지정
2. **경고 임계값 설정**:
   - 응답시간: 100ms (낮을수록 좋음)
   - 손실률: 2.0% (0%에 가까울수록 좋음)
   - 지연시간: 10초 (문제 지속 시 알림)

## 📊 사용법

### 실시간 모니터링

1. **GUI 실행**: `python main.py`
2. **플랫폼 선택**: 드롭다운에서 스트리밍 플랫폼 선택
3. **모니터링 시작**: 자동으로 메트릭 수집 및 표시
4. **권장 설정 확인**: 하단에 플랫폼별 최적 설정 표시

### 진단 모드

1. **진단 시작**: 상단 "진단 모드" 버튼 클릭
2. **진행 상황 확인**: 진행바와 실시간 메트릭 표시
3. **리포트 생성**: 완료 후 HTML/JSON/CSV 형식으로 저장
4. **결과 분석**: Documents/LiveOpsReports 폴더에서 확인

### 플랫폼별 권장 설정

| 플랫폼 | 최대 비트레이트 | 키프레임 | 권장 인코더 |
|--------|----------------|----------|-------------|
| SOOP | 8,000 kbps | 1초 | NVENC/QSV |
| CHZZK | 8,000 kbps | 1초 | NVENC/QSV |
| YouTube | 15,000 kbps | 2초 | NVENC/QSV |
| Twitch | 6,000 kbps | 2초 | NVENC/QSV |
| 아프리카TV | 4,000 kbps | 1초 | x264/NVENC |

## ⚙️ 설정

### 백엔드 설정 (`config.txt`)

```
# 네트워크 설정
net.probe_host=8.8.8.8
net.interval_ms=1000

# UI 설정
ui.theme=dark
ui.simpleMode=true

# 플랫폼 설정
platform=soop
diag_minutes=60
webhook=

# 로깅 설정
logging.level=info
logging.file_enabled=true
logging.console_enabled=true
```

### GUI 설정 (`ui_py/settings.json`)

```json
{
  "backend_path": "C:\\Projects\\liveops-sentinel\\build\\Release\\liveops_backend.exe",
  "webhook": "",
  "thresholds": {
    "rttMs": 100,
    "lossPct": 2.0,
    "holdSec": 10
  }
}
```

## 📈 성능 지표

### 모니터링 정확도
- **응답시간 측정**: ±5ms 정확도
- **패킷 손실 감지**: 0.1% 단위 측정
- **대역폭 측정**: 실시간 업로드 속도 추적

### 시스템 요구사항
- **CPU**: 최소 1%, 평균 2-3%
- **메모리**: 약 50MB (백엔드 + GUI)
- **네트워크**: 1Hz 메트릭 수집

## 🔧 문제 해결

### 일반적인 문제

**Q: 백엔드 실행 파일을 찾을 수 없습니다**
A: 설정 마법사에서 수동으로 경로를 지정하거나, 빌드가 완료되었는지 확인하세요.

**Q: 네트워크 메트릭이 0으로 표시됩니다**
A: 관리자 권한으로 실행하거나, 방화벽 설정을 확인하세요.

**Q: GUI가 실행되지 않습니다**
A: Python 의존성이 설치되었는지 확인: `pip install -r requirements.txt`

### 성능 최적화

**네트워크 상태가 좋지 않을 때**:
- 비트레이트를 20% 낮추기
- 해상도를 한 단계 낮추기 (1080p → 720p)
- 유선 연결 사용 권장

**CPU 사용률이 높을 때**:
- HW 인코더 사용 (NVENC/QSV)
- 해상도 또는 FPS 낮추기
- 다른 프로그램 종료

## 🤝 기여하기

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📝 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 📞 지원

- **이슈 리포트**: [GitHub Issues](https://github.com/your-username/liveops-sentinel/issues)
- **문서**: [Wiki](https://github.com/your-username/liveops-sentinel/wiki)
- **릴리즈**: [Releases](https://github.com/your-username/liveops-sentinel/releases)

## 🗺️ 로드맵

### 단기 목표 (1-2개월)
- [x] 기본 모니터링 시스템
- [x] 플랫폼별 권장 설정
- [x] 진단 모드 및 리포트
- [ ] OBS Studio 연동
- [ ] ZIP 내보내기 기능

### 중기 목표 (3-6개월)
- [ ] Linux 지원
- [ ] 고급 분석 기능
- [ ] 웹 대시보드
- [ ] 모바일 앱

### 장기 목표 (6개월+)
- [ ] AI 기반 예측
- [ ] 분산 모니터링
- [ ] 클라우드 연동

---

**LiveOps Sentinel** - 안정적인 스트리밍을 위한 스마트 모니터링 솔루션 
