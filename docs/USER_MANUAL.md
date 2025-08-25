# LiveOps Sentinel 사용자 매뉴얼

## 목차
1. [소개](#소개)
2. [시작하기](#시작하기)
3. [기본 기능](#기본-기능)
4. [고급 기능](#고급-기능)
5. [설정](#설정)
6. [문제 해결](#문제-해결)
7. [FAQ](#faq)

## 소개

LiveOps Sentinel은 실시간 스트리밍 환경에서 네트워크, 시스템, OBS 상태를 모니터링하고 문제를 사전에 감지하여 알림을 제공하는 종합적인 모니터링 도구입니다.

### 주요 기능
- **실시간 네트워크 모니터링**: RTT, 패킷 손실, 대역폭 측정
- **시스템 리소스 모니터링**: CPU, GPU, 메모리 사용률 추적
- **OBS 통합**: 스트리밍/녹화 상태, 드롭된 프레임 모니터링
- **스마트 알림**: Discord, Slack, 이메일을 통한 실시간 알림
- **성능 최적화**: 자동 메모리 관리 및 CPU 최적화
- **업데이트 관리**: 자동 업데이트 확인 및 설치

## 시작하기

### 시스템 요구사항
- **운영체제**: Windows 10/11 (64비트)
- **메모리**: 최소 4GB RAM (권장 8GB)
- **저장공간**: 최소 100MB
- **네트워크**: 인터넷 연결 (알림 기능용)

### 설치 방법

#### 1. 설치 프로그램 사용 (권장)
1. [릴리스 페이지](https://github.com/liveops-sentinel/releases)에서 최신 버전 다운로드
2. `LiveOpsSentinel-Setup-x.x.x.exe` 실행
3. 설치 마법사의 안내에 따라 설치 진행
4. 설치 완료 후 바탕화면에서 바로가기 실행

#### 2. 포터블 버전 사용
1. `LiveOpsSentinel-Portable-x.x.x.zip` 다운로드
2. 원하는 폴더에 압축 해제
3. `start_liveops_sentinel.bat` 또는 `start_liveops_sentinel.ps1` 실행

### 첫 실행
1. 프로그램 실행 시 설정 마법사가 자동으로 시작됩니다
2. 기본 설정을 확인하고 필요에 따라 수정
3. Discord 웹훅 URL 입력 (선택사항)
4. OBS WebSocket 설정 (OBS 사용 시)
5. 설정 완료 후 모니터링 시작

## 기본 기능

### 대시보드
메인 화면에서 다음 정보를 실시간으로 확인할 수 있습니다:

#### 네트워크 상태
- **RTT (Round Trip Time)**: 네트워크 지연 시간
- **패킷 손실률**: 데이터 손실 비율
- **대역폭**: 현재 네트워크 속도
- **네트워크 품질**: A-F 등급으로 표시

#### 시스템 상태
- **CPU 사용률**: 전체 및 코어별 사용률
- **GPU 사용률**: 그래픽 카드 사용률
- **메모리 사용률**: RAM 사용량
- **디스크 사용률**: 저장공간 사용량

#### OBS 상태 (OBS 연동 시)
- **스트리밍 상태**: 현재 스트리밍 중인지 여부
- **녹화 상태**: 현재 녹화 중인지 여부
- **드롭된 프레임**: 인코딩 중 손실된 프레임 수
- **인코딩 지연**: 인코딩 처리 지연 시간

### 알림 시스템
임계값을 초과하면 자동으로 알림이 발송됩니다:

#### 알림 수준
- **정보 (INFO)**: 일반적인 상태 정보
- **경고 (WARNING)**: 주의가 필요한 상황
- **심각 (CRITICAL)**: 즉시 조치가 필요한 상황

#### 알림 채널
- **Discord**: 웹훅을 통한 실시간 메시지
- **Slack**: 워크스페이스 채널로 알림
- **이메일**: SMTP를 통한 이메일 발송

## 고급 기능

### 성능 최적화
LiveOps Sentinel은 자동으로 시스템 성능을 최적화합니다:

#### 메모리 관리
- **자동 캐시 정리**: 사용하지 않는 데이터 자동 삭제
- **메모리 누수 감지**: 메모리 누수 자동 탐지 및 보고
- **가비지 컬렉션**: 주기적인 메모리 정리

#### CPU 최적화
- **작업 스케줄링**: 백그라운드 작업 최적화
- **스레드 풀 관리**: 효율적인 스레드 사용
- **성능 프로파일링**: 작업별 성능 분석

### 네트워크 진단
고급 네트워크 분석 기능을 제공합니다:

#### 대역폭 테스트
- **업로드/다운로드 속도 측정**
- **대역폭 사용량 예측**
- **네트워크 병목 지점 식별**

#### 연결 품질 분석
- **ping 테스트**: 서버 응답 시간 측정
- **패킷 손실 분석**: 네트워크 안정성 평가
- **라우팅 경로 분석**: 네트워크 경로 최적화

### 업데이트 관리
자동 업데이트 시스템을 통해 최신 기능을 유지합니다:

#### 업데이트 채널
- **안정 (Stable)**: 검증된 안정 버전
- **베타 (Beta)**: 테스트 중인 기능 포함
- **개발 (Development)**: 최신 개발 버전

#### 업데이트 설정
- **자동 업데이트**: 자동으로 업데이트 확인 및 설치
- **수동 업데이트**: 사용자가 직접 업데이트 관리
- **롤백 기능**: 이전 버전으로 되돌리기

## 설정

### 기본 설정
`설정` 탭에서 다음 항목을 구성할 수 있습니다:

#### 알림 설정
```json
{
  "alerts": {
    "rtt_warning_ms": 50,
    "rtt_critical_ms": 100,
    "loss_warning_pct": 1.0,
    "loss_critical_pct": 5.0,
    "cpu_warning_pct": 80.0,
    "cpu_critical_pct": 95.0,
    "gpu_warning_pct": 80.0,
    "gpu_critical_pct": 95.0
  }
}
```

#### Discord 설정
```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/...",
    "enabled": true,
    "mention_role": "@everyone"
  }
}
```

#### OBS 설정
```json
{
  "obs": {
    "websocket_url": "ws://localhost:4444",
    "password": "",
    "auto_connect": true
  }
}
```

### 고급 설정
고급 사용자를 위한 세부 설정:

#### 성능 최적화 설정
```json
{
  "performance": {
    "memory_limit_mb": 1024,
    "cache_size_mb": 200,
    "optimization_interval_seconds": 60,
    "auto_optimization": true
  }
}
```

#### 로깅 설정
```json
{
  "logging": {
    "level": "info",
    "console_enabled": true,
    "file_enabled": true,
    "max_file_size_mb": 10,
    "max_files": 5
  }
}
```

## 문제 해결

### 일반적인 문제

#### 프로그램이 시작되지 않음
1. **관리자 권한으로 실행**: 프로그램을 관리자 권한으로 실행
2. **바이러스 백신 확인**: 바이러스 백신이 프로그램을 차단하지 않는지 확인
3. **의존성 확인**: Visual C++ 재배포 가능 패키지 설치

#### 네트워크 모니터링이 작동하지 않음
1. **방화벽 설정**: 프로그램이 네트워크에 접근할 수 있도록 허용
2. **관리자 권한**: 네트워크 인터페이스 접근 권한 확인
3. **네트워크 드라이버**: 네트워크 드라이버 업데이트

#### OBS 연동이 안됨
1. **OBS WebSocket 플러그인**: OBS에 WebSocket 플러그인 설치
2. **포트 설정**: WebSocket 포트(기본 4444) 확인
3. **비밀번호 설정**: OBS WebSocket 비밀번호 설정 확인

#### 알림이 발송되지 않음
1. **웹훅 URL 확인**: Discord/Slack 웹훅 URL 정확성 확인
2. **네트워크 연결**: 인터넷 연결 상태 확인
3. **알림 설정**: 알림 기능이 활성화되어 있는지 확인

### 로그 확인
문제 발생 시 로그 파일을 확인하여 원인을 파악할 수 있습니다:

#### 로그 파일 위치
- **Windows**: `%APPDATA%\LiveOps Sentinel\logs\`
- **설정 파일**: `%APPDATA%\LiveOps Sentinel\config.json`

#### 로그 레벨
- **DEBUG**: 상세한 디버깅 정보
- **INFO**: 일반적인 정보 메시지
- **WARNING**: 경고 메시지
- **ERROR**: 오류 메시지

## FAQ

### Q: LiveOps Sentinel은 무료인가요?
A: 네, LiveOps Sentinel은 완전히 무료로 제공됩니다.

### Q: 어떤 운영체제를 지원하나요?
A: 현재 Windows 10/11 (64비트)를 지원합니다. Linux 및 macOS 지원은 계획 중입니다.

### Q: OBS Studio가 필수인가요?
A: 아니요, OBS 연동은 선택사항입니다. 네트워크 및 시스템 모니터링은 OBS 없이도 사용 가능합니다.

### Q: 개인정보가 수집되나요?
A: 아니요, 모든 데이터는 로컬에서만 처리되며 외부로 전송되지 않습니다.

### Q: 업데이트는 자동으로 이루어지나요?
A: 설정에서 자동 업데이트를 활성화하면 자동으로 업데이트됩니다.

### Q: 성능에 영향을 주지 않나요?
A: LiveOps Sentinel은 최적화되어 있어 시스템 성능에 미치는 영향이 최소화됩니다.

### Q: 여러 모니터링 대상을 설정할 수 있나요?
A: 네, 여러 네트워크 인터페이스와 시스템 메트릭을 동시에 모니터링할 수 있습니다.

### Q: 알림을 커스터마이징할 수 있나요?
A: 네, 알림 메시지, 임계값, 발송 조건을 자유롭게 설정할 수 있습니다.

---

## 지원 및 문의

### 공식 채널
- **GitHub**: [https://github.com/liveops-sentinel](https://github.com/liveops-sentinel)
- **이슈 트래커**: [https://github.com/liveops-sentinel/issues](https://github.com/liveops-sentinel/issues)
- **릴리스**: [https://github.com/liveops-sentinel/releases](https://github.com/liveops-sentinel/releases)

### 커뮤니티
- **Discord**: [https://discord.gg/liveops-sentinel](https://discord.gg/liveops-sentinel)
- **Reddit**: [https://reddit.com/r/liveops-sentinel](https://reddit.com/r/liveops-sentinel)

### 기여하기
LiveOps Sentinel은 오픈소스 프로젝트입니다. 버그 리포트, 기능 제안, 코드 기여를 환영합니다.

---

**버전**: 1.0.0  
**최종 업데이트**: 2024년 12월  
**라이선스**: MIT License
