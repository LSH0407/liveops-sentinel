# LiveOps Sentinel

UE/OBS 라이브 송출 품질을 모니터링하는 C++ 데스크톱 애플리케이션 (MVP)

## 🎯 프로젝트 목표

라이브 스트리밍 환경에서 네트워크 품질과 OBS Studio 상태를 실시간으로 모니터링하여 안정적인 송출을 보장하는 도구입니다.

## ✨ 주요 기능

- **네트워크 모니터링**: UDP 프로브를 통한 RTT 및 패킷 손실률 측정
- **OBS Studio 연동**: WebSocket을 통한 실시간 OBS 상태 모니터링
- **프로세스 모니터링**: UE/OBS 프로세스의 CPU/메모리 사용량 추적
- **실시간 알림**: Discord Webhook을 통한 임계치 기반 알림
- **대역폭 벤치마크**: TCP/UDP 스루풋 측정 및 네트워크 품질 분석
- **OBS 설정 추천**: 현재 네트워크/하드웨어 상태에 맞춘 최적 비트레이트/설정 자동 산출
- **자동 진단 모드**: 이상 감지 시 상세 로깅 및 분석 리포트 생성
- **OBS 제어**: 씬 변경, 스트리밍/녹화 시작/중지, 이벤트 로그
- **임계치 경보**: 홀드 타임 기반 RTT/Loss 경보 및 최근 메트릭 첨부
- **Pre-flight 체크리스트**: 방송 전 자동 점검 (OBS 연결, UE 프로세스, NDI, 디스크 공간)
- **리포트 내보내기**: JSON/CSV 형식의 시간대별 메트릭 자동 저장
- **직관적인 UI**: Dear ImGui 기반의 실시간 대시보드

## 📥 다운로드

### 최신 릴리스
- **Windows ZIP**: [LiveOps Sentinel v0.2.0](https://github.com/LSH0407/liveops-sentinel/releases/latest)
- **Linux TGZ**: [LiveOps Sentinel v0.2.0](https://github.com/LSH0407/liveops-sentinel/releases/latest)

### 실행 방법
1. 다운로드한 ZIP 파일을 원하는 폴더에 압축 해제
2. `liveops_sentinel.exe` (Windows) 또는 `liveops_sentinel` (Linux) 실행
3. **첫 실행 시 Discord Webhook 설정 마법사가 자동으로 표시됩니다**

### 시스템 요구사항
- **Windows**: Windows 10/11 (x64)
- **Linux**: Ubuntu 20.04+ 또는 호환 배포판
- **메모리**: 최소 4GB RAM
- **디스크**: 최소 1GB 여유 공간

## 🛠 기술 스택

- **언어**: C++20
- **빌드 시스템**: CMake + vcpkg
- **GUI**: SDL2 + Dear ImGui
- **네트워킹**: asio (UDP 프로브, WebSocket)
- **JSON**: nlohmann-json
- **로깅**: spdlog
- **테스팅**: doctest

## 📋 요구사항

### Windows
- Visual Studio 2019 이상 또는 MSVC 14.29+
- CMake 3.22+
- vcpkg

### Linux
- GCC 10+ 또는 Clang 12+
- CMake 3.22+
- vcpkg

## 🚀 빌드 및 실행

### 1. 의존성 설치

```bash
# vcpkg 설치 (Windows)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install

# vcpkg 설치 (Linux)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

### 2. 프로젝트 빌드

```bash
# 프로젝트 클론
git clone <repository-url>
cd liveops-sentinel

# 빌드 디렉토리 생성
mkdir build && cd build

# CMake 설정
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# 빌드
cmake --build . --config Release
```

### 3. 실행

```bash
# Windows
./Release/liveops_sentinel.exe

# Linux
./liveops_sentinel
```

## 📊 사용법

### 1. 네트워크 프로브 설정
- **Host**: 대상 서버 IP 주소 (기본값: 127.0.0.1)
- **Port**: UDP 에코 서버 포트 (기본값: 50051)
- **Send Rate**: 초당 패킷 전송률 (1-60 Hz)

### 2. OBS Studio 연동
- **Host**: OBS Studio 호스트 (기본값: localhost)
- **Port**: OBS WebSocket 포트 (기본값: 4455)
- **Password**: OBS WebSocket 비밀번호 (설정된 경우)

### 3. 벤치마크 및 추천
- **대역폭 테스트**: 서버/클라이언트 모드로 TCP/UDP 스루풋 측정
- **OBS 설정 추천**: 네트워크 품질과 하드웨어 성능을 고려한 최적 설정 자동 계산
- **프리셋 저장**: 추천 설정을 JSON 형태로 저장하여 OBS에서 직접 적용 가능

### 4. 알림 설정
- **RTT Threshold**: RTT 임계값 (ms)
- **Loss Threshold**: 패킷 손실률 임계값 (%)
- **Dropped Frames Threshold**: 프레임 드롭 임계값 (%)
- **Discord Webhook**: Discord 알림 URL

## 🧪 테스트

```bash
# 단위 테스트 실행
cd build
ctest --output-on-failure

# 또는 직접 실행
./tests/unit_tests
```

## 📁 프로젝트 구조

```
liveops-sentinel/
├── src/
│   ├── app/          # 애플리케이션 메인
│   ├── ui/           # 사용자 인터페이스
│   ├── net/          # 네트워크 프로브
│   ├── obs/          # OBS WebSocket 클라이언트
│   ├── sys/          # 시스템 모니터링
│   ├── alert/        # 알림 시스템
│   ├── diag/         # 진단 및 벤치마크
│   └── core/         # 핵심 유틸리티
├── tests/            # 단위 테스트
├── assets/           # 리소스 파일
└── external/         # 외부 의존성
```

## 🔧 설정

애플리케이션은 `config.json` 파일을 통해 설정을 관리합니다:

```json
{
  "probeHost": "127.0.0.1",
  "probePort": 50051,
  "probeRateHz": 20,
  "obsHost": "localhost",
  "obsPort": 4455,
  "obsPassword": "",
  "discordWebhook": "https://discord.com/api/webhooks/...",
  "rttThreshold": 100.0,
  "lossThreshold": 5.0,
  "enableDiscord": true,
  "monitoredProcesses": ["obs64.exe", "UnrealEditor.exe"],
  "bench": {
    "durationSec": 30,
    "proto": "udp",
    "headroom": 0.75,
    "minKbps": 800,
    "maxKbps": 15000
  },
  "thresholds": {
    "rttMs": 80.0,
    "lossPct": 1.0,
    "droppedFramesRatio": 0.02,
    "encodingLagMs": 25.0,
    "renderLagMs": 20.0,
    "alertHoldSec": 5
  }
}
```

## 🎮 JD 매핑

### 시니어 C++ 엔지니어 역량

- **C++20 고급 기능 활용**: Concepts, Ranges, Coroutines 등
- **멀티스레딩 프로그래밍**: asio 기반 비동기 네트워킹
- **크로스 플랫폼 개발**: Windows/Linux 빌드 지원
- **모듈화 설계**: SOLID 원칙 적용
- **테스트 주도 개발**: doctest 기반 단위 테스트
- **성능 최적화**: 메모리 효율적 데이터 구조

### 라이브 스트리밍 도메인 지식

- **네트워크 프로토콜**: UDP 프로브, WebSocket, TCP/UDP 벤치마크
- **미디어 스트리밍**: OBS Studio API 연동, 인코더 설정 최적화
- **실시간 모니터링**: 지연시간, 패킷 손실, 프레임 드롭 측정
- **알림 시스템**: Discord Webhook 통합, 자동 진단 모드
- **성능 분석**: 대역폭 벤치마크, 하드웨어 리소스 모니터링

## 📊 비트레이트 추천 시스템

### 알고리즘 개요

LiveOps Sentinel은 다음과 같은 요소들을 종합적으로 분석하여 최적의 OBS 설정을 추천합니다:

1. **네트워크 품질 분석**
   - 지속 가능한 업링크 대역폭 (Mbps)
   - RTT (Round Trip Time) 및 지터
   - 패킷 손실률

2. **하드웨어 성능 평가**
   - CPU/GPU 사용률
   - 인코딩 지연시간
   - 렌더링 지연시간

3. **알고리즘 로직**
   ```
   safeBitrate = sustainedUplinkMbps × headroom × networkQualityFactor
   recommendedBitrate = clamp(safeBitrate, minKbps, maxKbps)
   ```

### 사용법

1. **벤치마크 실행**
   - View → Benchmark 메뉴 선택
   - 서버/클라이언트 모드 설정
   - 테스트 시간 및 프로토콜 선택
   - "Start Benchmark" 클릭

2. **추천값 확인**
   - 벤치마크 완료 후 자동으로 추천 설정 계산
   - 인코더, 비트레이트, 키프레임 간격, VBV 버퍼 등 표시
   - 상세한 노트와 함께 설정 근거 제공

3. **프리셋 저장**
   - "Apply as Preset (JSON)" 버튼 클릭
   - `reports/obs_preset_YYYYMMDD_HHMM.json` 파일로 저장
   - OBS Studio에서 직접 import 가능

### 서버/클라이언트 테스트 모드

**서버 모드**: 다른 클라이언트의 연결을 받아 대역폭 측정
- 방화벽에서 해당 포트 개방 필요
- 수신 대역폭 및 패킷 처리량 측정

**클라이언트 모드**: 지정된 서버로 연결하여 업링크 측정
- 대상 서버 IP/포트 설정
- 전송 대역폭, 손실률, RTT 측정

### 지표 해석 가이드

- **RTT < 50ms**: 우수한 네트워크 품질
- **RTT 50-100ms**: 양호한 네트워크 품질
- **RTT > 100ms**: 네트워크 지연 주의
- **Loss < 1%**: 안정적인 연결
- **Loss 1-5%**: 주의 필요
- **Loss > 5%**: 네트워크 문제 가능성 높음

## 🎛️ OBS 제어 및 이벤트 로그

### OBS WebSocket 연동

LiveOps Sentinel은 OBS Studio의 WebSocket API를 통해 실시간 제어 및 모니터링을 제공합니다.

#### 주요 기능
- **씬 제어**: 드롭다운을 통한 현재 프로그램 씬 변경
- **스트리밍 제어**: 원클릭 스트리밍 시작/중지
- **녹화 제어**: 원클릭 녹화 시작/중지
- **이벤트 로그**: OBS 이벤트의 실시간 기록 및 검색

#### 사용법
1. **OBS Studio 설정**
   - Tools → WebSocket Server Settings
   - Enable WebSocket server 체크
   - Port: 4455 (기본값)
   - Password 설정 (선택사항)

2. **연결 확인**
   - Control 탭에서 연결 상태 확인
   - 녹색 상태 표시 시 제어 가능

3. **씬 변경**
   - Scene Control 섹션에서 원하는 씬 선택
   - Apply 버튼 클릭

4. **이벤트 로그**
   - 최근 200개 이벤트 실시간 표시
   - 검색 필터로 특정 이벤트 찾기
   - JSON 형식으로 로그 저장

## ⚠️ 임계치 경보 시스템

### 홀드 타임 기반 경보

네트워크 품질 지표가 일정 시간 동안 임계치를 초과할 때만 경보를 발생시켜 허위 알림을 방지합니다.

#### 기본 임계치 설정
- **RTT**: 80ms (5초 홀드)
- **패킷 손실**: 2% (5초 홀드)
- **프레임 드롭**: 2% (5초 홀드)
- **인코딩 지연**: 25ms (5초 홀드)
- **렌더링 지연**: 20ms (5초 홀드)

#### 경보 메시지 구성
```
⚠️ WARNING **High RTT Detected**
RTT exceeded threshold for 5 seconds. Avg: 95.2 ms, Max: 120.5 ms

**Recent Metrics (10s snapshot):**
```json
{
  "rtt": {"avg_ms": 95.2, "max_ms": 120.5},
  "loss": {"avg_pct": 1.2, "max_pct": 3.1},
  "obs": {
    "droppedFramesRatio": 0.015,
    "encodingLagMs": 18.5,
    "renderLagMs": 12.3,
    "cpu_pct": 65.2
  }
}
```
```

#### 설정 튜닝
- `config.json`의 `thresholds` 섹션에서 값 조정
- `holdSec` 값을 늘려서 더 안정적인 경보 설정
- Discord Webhook URL 설정으로 알림 수신

## ✅ Pre-flight 체크리스트

### 자동 점검 항목

방송 시작 전 시스템 상태를 자동으로 점검하여 문제를 사전에 발견합니다.

#### 점검 항목
1. **OBS 연결 상태**
   - WebSocket 연결 및 인증 성공 여부
   - 연결 실패 시 OBS Studio 실행 및 WebSocket 활성화 안내

2. **OBS 성능 상태**
   - 현재 FPS 및 출력 상태 정상 여부
   - 프레임 드롭률 및 렌더링 지연시간 확인

3. **UE 프로세스 확인**
   - UnrealEditor.exe 또는 UE4Editor.exe 실행 상태
   - 프로세스 미실행 시 경고 메시지

4. **NDI 입력 소스**
   - NDI 플러그인 및 소스 존재 여부
   - 수동 확인 필요 시 안내 메시지

5. **디스크 여유 공간**
   - 녹화 디렉토리 최소 10GB 여유 공간 확인
   - 부족 시 경고 및 정리 안내

#### 사용법
1. **체크리스트 실행**
   - View → Checklist 메뉴 선택
   - "Run Pre-flight" 버튼 클릭

2. **결과 확인**
   - ✅ 녹색: 정상
   - ⚠️ 노란색: 경고 (수동 확인 필요)
   - ❌ 빨간색: 실패 (즉시 해결 필요)

3. **결과 저장**
   - "Copy Result": 클립보드에 JSON 복사
   - "Save JSON": `reports/preflight_YYYYMMDD_HHMM.json` 저장

## 📊 리포트 시스템

### 자동 메트릭 수집

프로그램 실행 중 핵심 지표를 지속적으로 수집하여 분석 가능한 형태로 저장합니다.

#### 수집 지표
- **네트워크**: RTT, 패킷 손실률
- **OBS**: 프레임 드롭률, 렌더링 지연시간
- **시스템**: CPU 사용률, GPU 사용률, 메모리 사용량

#### 파일 형식

**CSV 형식** (`metrics_YYYYMMDD_HHMM.csv`)
```csv
ts,rtt_ms,loss_pct,obs_dropped_ratio,avg_render_ms,cpu_pct,gpu_pct,mem_mb
2025-01-15 14:30:00.123,15.2,0.1,0.012,14.8,45.2,65.1,2048.5
2025-01-15 14:30:01.124,16.1,0.2,0.015,15.1,46.1,66.2,2050.1
```

**JSON 형식** (`metrics_YYYYMMDD_HHMM.json`)
```json
{
  "metadata": {
    "exportTime": 1642248600,
    "totalSnapshots": 600,
    "flushIntervalSec": 10
  },
  "snapshots": [
    {
      "timestamp": 1642248600123,
      "rtt_ms": 15.2,
      "loss_pct": 0.1,
      "obs_dropped_ratio": 0.012,
      "avg_render_ms": 14.8,
      "cpu_pct": 45.2,
      "gpu_pct": 65.1,
      "mem_mb": 2048.5
    }
  ]
}
```

#### 사용법
1. **자동 저장**
   - 기본 10초마다 자동 저장
   - `config.json`의 `report.flushIntervalSec`에서 주기 조정

2. **수동 저장**
   - Reports 탭에서 "Flush Now" 버튼 클릭
   - 즉시 현재까지의 데이터 저장

3. **파일 관리**
   - "Open Folder" 버튼으로 reports 디렉토리 열기
   - 최근 20개 파일 목록 표시
   - 클릭으로 파일 탐색기에서 열기

#### 설정 옵션
```json
{
  "report": {
    "enable": true,
    "flushIntervalSec": 10,
    "dir": "reports",
    "maxFileSizeMB": 25
  }
}
```

## 🔍 자동 진단 모드

### 이상 감지 시 고주기 로깅

네트워크 품질이 지속적으로 저하될 때 자동으로 진단 모드가 활성화되어 상세한 분석 데이터를 수집합니다.

#### 진입 조건
- RTT > 80ms + Loss > 1% 지속 (5초)
- 프레임 드롭률 > 2% 지속
- 인코딩/렌더링 지연 지속

#### 진단 모드 특징
- **고주기 샘플링**: 네트워크 프로브 20Hz → 60Hz로 증가
- **상세 지표 수집**: OBS Stats, CPU/GPU/메모리/디스크 사용량
- **자동 종료**: 3-5분 후 자동 종료 및 리포트 생성
- **실시간 알림**: 진단 시작/종료 시 Discord 알림

#### 진단 리포트
- **JSON 형식**: `reports/diag_YYYYMMDD_HHMM.json`
- **CSV 형식**: `reports/diag_YYYYMMDD_HHMM.csv`
- **통계 요약**: 평균/최대값, 지속 시간, 샘플 수

#### 리포트 예시
```json
{
  "metadata": {
    "startTime": 1642248600,
    "endTime": 1642248900,
    "durationSec": 300,
    "totalSamples": 18000,
    "samplingRateHz": 60
  },
  "summary": {
    "avgRtt": 95.2,
    "maxRtt": 150.5,
    "avgLoss": 2.1,
    "maxLoss": 5.3,
    "avgDroppedFrames": 0.025,
    "maxDroppedFrames": 0.08,
    "avgCpu": 75.2,
    "maxCpu": 95.1
  },
  "samples": [
    {
      "timestamp": 1642248600123,
      "rtt_ms": 95.2,
      "loss_pct": 2.1,
      "droppedFramesRatio": 0.025,
      "encodingLagMs": 28.5,
      "renderLagMs": 22.1,
      "cpu_pct": 75.2,
      "gpu_pct": 65.1,
      "mem_mb": 2048.5,
      "diskWriteMBps": 120.5
    }
  ]
}
```

## 🎯 벤치마크 & 권장 설정

### 대역폭 측정 및 OBS 설정 최적화

네트워크 대역폭을 측정하고 현재 시스템 상태에 맞는 최적의 OBS 설정을 자동으로 추천합니다.

#### 벤치마크 기능
- **TCP/UDP 지원**: 두 프로토콜 모두 측정 가능
- **서버/클라이언트 모드**: 로컬 또는 원격 측정
- **시스템 지표 수집**: CPU/GPU/디스크 사용량 동시 측정
- **손실률/지터 측정**: 네트워크 품질 종합 분석

#### 권장 설정 알고리즘
1. **안전 비트레이트 계산**: `sustainedUplinkMbps * headroom`
2. **네트워크 불안정성 반영**: RTT/Loss에 따른 추가 헤드룸 적용
3. **해상도별 클램핑**: 720p/1080p/1440p/4K별 권장 범위 적용
4. **인코더 선택**: CPU/GPU 부하에 따른 최적 인코더 선택
5. **프리셋 최적화**: 시스템 리소스에 따른 품질/성능 균형

#### Headroom 개념
- **기본값**: 0.75 (75% 사용, 25% 여유)
- **네트워크 불안정 시**: 0.6-0.7로 자동 축소
- **안정적 네트워크**: 0.8까지 확장 가능

#### 권장 설정 예시
```json
{
  "encoder": "nvenc",
  "bitrateKbps": 7000,
  "keyframeSec": 2,
  "vbvBufferKbps": 7000,
  "preset": "quality",
  "profile": "main",
  "notes": "headroom 0.75; loss 0.3% → 유지 가능"
}
```

#### 사용법
1. **벤치마크 실행**
   - Benchmark 탭에서 서버/클라이언트 모드 선택
   - 프로토콜(UDP/TCP) 및 지속 시간 설정
   - "Start Benchmark" 버튼 클릭

2. **권장 설정 확인**
   - 벤치마크 완료 후 자동으로 권장 설정 계산
   - 비트레이트, 키프레임, 버퍼 크기 등 표시
   - 인코더 및 프리셋 권장사항 확인

3. **프리셋 저장**
   - "Save as OBS preset(JSON)" 버튼 클릭
   - `reports/obs_preset_YYYYMMDD_HHMM.json` 저장
   - OBS Studio에서 직접 적용 가능

## ⚠️ 알림 시스템 고도화

### 쿨다운 및 집계 기능

동일한 유형의 알림이 반복 발생할 때 쿨다운 기간을 적용하여 알림 스팸을 방지합니다.

#### 쿨다운 기능
- **기본 쿨다운**: 60초 (설정 가능)
- **알림 타입별 분리**: RTT, Loss, Encode Lag 등 독립적 쿨다운
- **집계 메시지**: 쿨다운 기간 내 다중 이벤트를 하나로 합쳐서 전송

#### 집계 정보
- **발생 횟수**: 쿨다운 기간 내 총 발생 횟수
- **지속 시간**: 첫 발생부터 마지막 발생까지의 시간
- **최대값/평균값**: 수치 지표의 통계 정보

#### 설정 옵션
```json
{
  "thresholds": {
    "rttMs": 80.0,
    "lossPct": 2.0,
    "holdSec": 5,
    "cooldownSec": 60
  }
}
```

## 🔧 Pre-flight 체크리스트 보강

### 추가 점검 항목

기존 Pre-flight 체크리스트에 네트워크 어댑터와 디스크 성능 점검을 추가했습니다.

#### Wi-Fi 연결 경고
- **활성화 조건**: `preflight.warnIfWifi = true`
- **경고 메시지**: "Wi-Fi 연결 감지됨 - 유선 연결 권장"
- **권장사항**: 안정성을 위해 유선 연결 사용

#### 디스크 쓰기 속도 측정
- **측정 방법**: 64MB 테스트 파일 생성/삭제
- **권장 기준**: ≥100 MB/s
- **경고 기준**: 50-100 MB/s
- **실패 기준**: <50 MB/s

#### 설정 옵션
```json
{
  "preflight": {
    "ueProcessHints": ["UnrealEditor.exe", "UE4Editor.exe"],
    "ndiInputKindHint": "ndi",
    "diskMinGB": 10,
    "warnIfWifi": true
  }
}
```

## 📊 리포트 시스템 향상

### 파일 크기 제한 및 프라이버시 보호

대용량 파일 관리와 개인정보 보호 기능을 추가했습니다.

#### 파일 크기 제한
- **기본 제한**: 25MB (설정 가능)
- **자동 롤오버**: 크기 초과 시 새 파일로 자동 전환
- **파일명 패턴**: `metrics_YYYYMMDD_HHMM_part2.csv`

#### 프라이버시 보호
- **사용자명 제거**: Windows 사용자 경로에서 사용자명 마스킹
- **호스트명 제거**: 컴퓨터명 패턴 마스킹
- **PII 필터링**: 개인식별정보 자동 제거

#### 설정 옵션
```json
{
  "report": {
    "enable": true,
    "flushIntervalSec": 10,
    "dir": "reports",
    "maxFileSizeMB": 25
  }
}
```

## 💡 운영 팁

### 최적의 설정 가이드

#### 네트워크 설정
- **유선 연결 권장**: Wi-Fi보다 안정적인 유선 연결 사용
- **헤드룸 설정**: 0.7-0.8 범위에서 시작하여 조정
- **임계치 조정**: 네트워크 환경에 맞게 RTT/Loss 임계치 설정

#### 하드웨어 최적화
- **디스크 성능**: 연속 쓰기 속도 ≥100MB/s 권장
- **CPU/GPU 모니터링**: 85% 이상 시 프리셋 완화 고려
- **메모리 여유**: 최소 4GB 이상 여유 메모리 확보

#### OBS 설정
- **인코더 선택**: GPU 여유 시 NVENC, CPU 여유 시 x264
- **키프레임 간격**: 네트워크 불안정 시 1-2초로 단축
- **버퍼 크기**: 불안정한 네트워크에서 0.5-0.8배로 축소

## 🤝 기여하기

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 🔗 Repository

<!--REPO_URL_START-->
https://github.com/LSH0407/liveops-sentinel
<!--REPO_URL_END-->

## 🚀 첫 실행 및 Webhook 설정

### 첫 실행 시 자동 설정

LiveOps Sentinel을 처음 실행하면 Discord Webhook 설정 마법사가 자동으로 표시됩니다.

#### 설정 과정
1. **웹훅 URL 입력**: Discord 서버에서 생성한 Webhook URL 입력
2. **URL 유효성 검사**: 자동으로 Discord Webhook 형식 검증
3. **테스트 전송**: "웹훅 테스트" 버튼으로 실제 메시지 전송 확인
4. **설정 저장**: 유효한 URL이면 자동으로 사용자 설정에 저장

#### 설정 저장 위치
- **Windows**: `%APPDATA%/LiveOpsSentinel/config.json`
- **Linux**: `$XDG_CONFIG_HOME/liveops-sentinel/config.json` (fallback: `~/.config/liveops-sentinel/config.json`)

#### CLI 옵션
```bash
# 웹훅 URL 설정
./liveops_sentinel --webhook="https://discord.com/api/webhooks/ID/TOKEN"

# 웹훅 설정 초기화
./liveops_sentinel --reset-webhook

# 도움말
./liveops_sentinel --help
```

### 설정 관리

#### Settings 패널
- **View → Settings** 메뉴에서 웹훅 설정 관리
- **Change Webhook**: 새로운 웹훅 URL로 변경
- **Copy URL**: 마스킹된 URL을 클립보드에 복사
- **Reset Webhook**: 웹훅 설정 초기화
- **Test Webhook**: 현재 설정으로 테스트 메시지 전송

#### 보안 고려사항
- 웹훅 URL은 평문으로 저장 (암호화 불필요)
- UI에서는 마스킹된 형태로 표시 (첫 32자 + ****)
- 로그에는 마스킹된 URL만 기록
- 설정 파일은 리포지토리 외부에 저장

## 🔧 문제 해결

### Webhook 관련 문제

#### 테스트 실패 시 해결 방법
1. **방화벽 확인**: Discord API (443 포트) 접근 허용
2. **프록시 설정**: 회사 네트워크에서 프록시 설정 확인
3. **URL 형식**: `https://discord.com/api/webhooks/ID/TOKEN` 형식 확인
4. **웹훅 권한**: Discord 서버에서 웹훅 메시지 전송 권한 확인

#### 네트워크 오류
- **연결 실패**: 인터넷 연결 상태 확인
- **타임아웃**: 네트워크 지연 또는 방화벽 문제
- **재시도**: 자동으로 3회 지수 백오프 재시도

### 일반적인 문제

#### OBS 연결 실패
- OBS Studio 실행 확인
- obs-websocket 플러그인 설치 및 활성화
- 포트 4455 (기본값) 방화벽 허용

#### 프로브 연결 실패
- 대상 서버 실행 확인
- UDP 포트 50051 방화벽 허용
- 네트워크 연결 상태 확인

## 📞 지원

문제가 발생하거나 질문이 있으시면 [Issues](../../issues) 페이지를 통해 문의해 주세요.

---

**LiveOps Sentinel** - 안정적인 라이브 스트리밍을 위한 모니터링 솔루션 
