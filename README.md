# LiveOps Sentinel 🚀

**실시간 스트리밍 품질 모니터링 및 최적화 솔루션**

[![Python](https://img.shields.io/badge/Python-3.8+-blue.svg)](https://www.python.org/)
[![Qt](https://img.shields.io/badge/Qt-PySide6-green.svg)](https://doc.qt.io/qtforpython/)
[![C++](https://img.shields.io/badge/C++-17-red.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 📋 목차

- [개요](#개요)
- [주요 기능](#주요-기능)
- [모니터링 요소](#모니터링-요소)
- [시스템 요구사항](#시스템-요구사항)
- [설치 및 실행](#설치-및-실행)
- [사용법](#사용법)
- [설정](#설정)
- [품질 점수 계산](#품질-점수-계산)
- [문제 해결](#문제-해결)
- [개발 참여](#개발-참여)
- [라이선스](#라이선스)

## 🎯 개요

LiveOps Sentinel은 실시간 스트리밍 환경에서 네트워크, 시스템, OBS 성능을 종합적으로 모니터링하고 최적화하는 전문 도구입니다. 9개의 핵심 지표를 실시간으로 분석하여 스트리밍 품질을 점수화하고 구체적인 개선 방안을 제시합니다.

### 🏗️ 아키텍처

- **GUI (Python/PySide6)**: 현대적인 다크 테마 UI, 실시간 그래프, 통합 설정 관리
- **백엔드 (C++17)**: 고성능 메트릭 수집 및 분석 엔진
- **OBS 연동**: WebSocket v5 프로토콜을 통한 실시간 OBS 메트릭 수집
- **알림 시스템**: Discord 웹후크, 스트림 끊김 감지, 품질 저하 알림
- **스트림 헬스 모니터**: 실시간 스트리밍 상태 추적 및 분석

## ✨ 주요 기능

### 🔍 **실시간 모니터링**
- **9개 핵심 지표** 실시간 추적
- **연속 그래프** 표시 (60초 히스토리)
- **품질 점수** 자동 계산 (0-100점)
- **상황별 권장사항** 제공
- **스트림 헬스 모니터링** 추가

### 📊 **진단 및 벤치마크**
- **진단 모드**: 30초~180분 설정 가능
- **시스템 분석**: 최적 설정 자동 제안
- **진행률 표시**: 실시간 진행 상황
- **결과 리포트**: 상세 분석 결과
- **ZIP 내보내기**: 진단 결과 저장 기능

### 🎮 **OBS Studio 연동**
- **WebSocket v5** 프로토콜 지원
- **실시간 메트릭**: Dropped frames, Encoding/Render lag
- **연결 테스트**: 원클릭 연결 확인
- **자동 재연결**: 연결 끊김 시 자동 복구
- **OBS 설정 표시**: 현재 OBS 설정 정보 실시간 확인

### 🚨 **알림 시스템**
- **스트림 끊김 감지**: 시청자 경험 영향 분석
- **품질 저하 알림**: 실시간 경고 시스템
- **Discord 연동**: 웹후크를 통한 원격 알림
- **복구 알림**: 정상 상태 복구 시 알림

### 🎨 **현대적 UI/UX**
- **다크 테마**: 전문가급 디자인
- **반응형 레이아웃**: 창 크기 조정 지원
- **도움말 시스템**: 9개 지표 상세 설명
- **통합 설정 관리**: 모든 설정을 한 곳에서 관리
- **OBS 설정 디스플레이**: 현재 OBS 상태 실시간 표시

## 📈 모니터링 요소

### 🌐 **네트워크 지표 (3개)**
1. **서버 응답 속도 (RTT)**
   - 측정: ICMP ping을 통한 왕복 시간
   - 기준: 20ms(좋음) ~ 300ms(불량)
   - 영향: 스트림 지연, 버퍼링

2. **전송 손실률 (Packet Loss)**
   - 측정: UDP 패킷 손실률 분석
   - 기준: 0%(좋음) ~ 5%(불량)
   - 영향: 화질 저하, 끊김 현상

3. **업로드 여유율 (Uplink Headroom)**
   - 측정: 현재 비트레이트 대비 여유 대역폭
   - 기준: 50% 이상(좋음) ~ 0% 이하(불량)
   - 영향: 비트레이트 적응성

### 💻 **시스템 지표 (2개)**
4. **CPU 사용률**
   - 측정: 전체 CPU 사용률 모니터링
   - 기준: 60% 이하(좋음) ~ 90% 이상(불량)
   - 영향: 인코딩 성능, 시스템 안정성

5. **GPU 사용률**
   - 측정: 그래픽 카드 사용률 추적
   - 기준: 70% 이하(좋음) ~ 95% 이상(불량)
   - 영향: 렌더링 성능, 화질

### 📹 **OBS 지표 (4개)**
6. **버린 프레임 비율 (Dropped Frames)**
   - 측정: OBS WebSocket을 통한 실시간 수집
   - 기준: 1% 이하(좋음) ~ 5% 이상(불량)
   - 영향: 시청자 화질, 끊김 현상

7. **인코딩 지연 (Encoding Lag)**
   - 측정: 인코더 처리 시간 분석
   - 기준: 5ms 이하(좋음) ~ 20ms 이상(불량)
   - 영향: 실시간성, 동기화

8. **렌더 지연 (Render Lag)**
   - 측정: GPU 렌더링 시간 추적
   - 기준: 7ms 이하(좋음) ~ 25ms 이상(불량)
   - 영향: 화면 지연, 성능

9. **스트림 상태 (Stream Status)**
   - 측정: OBS 스트리밍 상태 모니터링
   - 기준: 정상 스트리밍(좋음) ~ 끊김(불량)
   - 영향: 전체 스트리밍 안정성

## 💻 시스템 요구사항

### 최소 요구사항
- **OS**: Windows 10/11 (64-bit)
- **CPU**: Intel i3-6100 / AMD Ryzen 3 1200 이상
- **RAM**: 4GB 이상
- **GPU**: DirectX 11 지원 그래픽 카드
- **네트워크**: 10Mbps 업로드 이상

### 권장 사양
- **OS**: Windows 11 (64-bit)
- **CPU**: Intel i5-8400 / AMD Ryzen 5 2600 이상
- **RAM**: 8GB 이상
- **GPU**: NVIDIA GTX 1060 / AMD RX 580 이상
- **네트워크**: 50Mbps 업로드 이상

### 소프트웨어 요구사항
- **Python**: 3.8 이상
- **OBS Studio**: 28.0 이상 (WebSocket Server 활성화 필요)
- **Visual Studio**: 2019 이상 (C++ 백엔드 빌드용)

## 🚀 설치 및 실행

### 1. 저장소 클론
```bash
git clone https://github.com/LSH0407/liveops-sentinel.git
cd liveops-sentinel
```

### 2. Python 환경 설정
```bash
# 가상환경 생성 및 활성화
python -m venv .venv
.venv\Scripts\activate  # Windows
source .venv/bin/activate  # Linux/Mac

# 의존성 설치
pip install -r ui_py/requirements.txt
```

### 3. C++ 백엔드 빌드
```bash
# 빌드 디렉토리 생성
mkdir build_backend
cd build_backend

# CMake 설정
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake

# 빌드 실행
cmake --build . --config Release
```

### 4. 실행
```bash
cd ui_py
python main.py
```

## 📖 사용법

### 🎯 기본 사용법

1. **프로그램 실행**
   - `ui_py/main.py` 실행
   - 초기 설정 마법사 진행

2. **백엔드 설정**
   - 설정 → 백엔드 탭
   - `build_backend/Release/liveops_backend.exe` 경로 지정

3. **OBS 연동 설정**
   - OBS Studio에서 WebSocket Server 활성화
   - 설정 → OBS 연동 탭에서 연결 정보 입력
   - "OBS 연결 테스트" 버튼으로 확인

4. **모니터링 시작**
   - "모니터링 시작" 버튼 클릭
   - 실시간 그래프 및 품질 점수 확인

### 🔧 고급 기능

#### 진단 모드
- **진행**: "진단 모드" 버튼 클릭
- **시간 설정**: 30초~180분 선택 가능
- **결과**: 시스템 최적화 권장사항 제공
- **내보내기**: ZIP 파일로 진단 결과 저장

#### 알림 설정
- **Discord**: 웹후크 URL 설정
- **임계값**: 경고 기준값 조정
- **자동 알림**: 스트림 끊김 시 자동 알림

#### 도움말 시스템
- **❓ 버튼**: 9개 지표 상세 설명
- **측정 방법**: 각 지표의 계산 방식
- **권장사항**: 상황별 개선 방안

#### OBS 설정 모니터링
- **실시간 설정 표시**: 현재 OBS 설정 정보 확인
- **연결 상태**: OBS WebSocket 연결 상태 실시간 표시
- **메트릭 수집**: OBS 성능 데이터 자동 수집

## ⚙️ 설정

### 백엔드 설정
- **실행 파일 경로**: `liveops_backend.exe` 위치
- **자동 시작**: 프로그램 실행 시 백엔드 자동 시작

### 알림 설정
- **Discord Webhook**: 알림 전송 URL
- **임계값 조정**: RTT, 손실률, 지연시간 기준

### OBS 연동 설정
- **서버 IP**: 기본값 `127.0.0.1`
- **포트**: 기본값 `4455`
- **비밀번호**: OBS WebSocket 설정과 일치
- **TLS**: 보안 연결 사용 여부
- **자동 재연결**: 연결 끊김 시 자동 복구

### 진단 설정
- **진단 시간**: 30초, 60초, 3분, 10분, 30분, 60분, 120분, 180분
- **분석 범위**: 시스템 전체 성능 분석
- **결과 저장**: ZIP 파일로 진단 결과 내보내기

## 📊 품질 점수 계산

### 점수 구성 (100점 만점)
- **네트워크 점수 (40%)**: RTT, 손실률, 업로드 여유율
- **시스템 점수 (30%)**: CPU, GPU 사용률
- **OBS 점수 (30%)**: Dropped frames, Encoding/Render lag

### 등급 기준
- **좋음 (85-100점)**: 최적 상태, 지속 모니터링
- **주의 (60-84점)**: 개선 필요, 권장사항 확인
- **불안정 (0-59점)**: 즉시 조치 필요

### 권장사항 시스템
- **네트워크 문제**: 비트레이트 조정, 유선 전환
- **OBS 성능**: 해상도 낮추기, 인코더 변경
- **시스템 부하**: HW 인코더 사용, 설정 최적화

## 🔧 문제 해결

### 일반적인 문제

#### 백엔드 연결 실패
```
문제: "백엔드 연결 실패" 메시지
해결: 
1. build_backend/Release/liveops_backend.exe 존재 확인
2. 설정에서 경로 재지정
3. 방화벽 예외 추가
```

#### OBS 연결 실패
```
문제: "OBS 연결 테스트" 실패
해결:
1. OBS Studio 실행 확인
2. 도구 → WebSocket Server Settings 활성화
3. 포트/비밀번호 설정 확인
4. TLS 설정 일치 확인
```

#### 그래프가 그려지지 않음
```
문제: 그래프에 데이터가 표시되지 않음
해결:
1. 모니터링 시작 확인
2. 백엔드 프로세스 실행 상태 확인
3. 네트워크 연결 상태 확인
```

### 성능 최적화

#### 높은 CPU 사용률
- 해상도를 720p로 낮추기
- HW 인코더(NVENC/QuickSync) 사용
- 불필요한 OBS 필터 제거

#### 네트워크 문제
- 유선 연결로 전환
- 공유기 QoS 설정 확인
- 비트레이트 20% 감소

#### OBS 성능 문제
- 캡처 소스 동시활성 최소화
- 필터 수 줄이기
- NVENC 인코더 사용

## 🤝 개발 참여

### 프로젝트 구조
```
liveops-sentinel/
├── ui_py/                 # Python GUI 애플리케이션
│   ├── main.py           # 메인 실행 파일
│   ├── views/            # UI 뷰 컴포넌트
│   ├── core/             # 핵심 로직 (OBS 클라이언트, 메트릭 버스 등)
│   ├── widgets/          # 커스텀 위젯
│   ├── actions/          # 사용자 액션
│   └── attic/            # 이전 버전 파일들 (아카이브)
├── src/                  # C++ 백엔드 소스
│   ├── main.cpp          # 메인 실행 파일
│   ├── core/             # 핵심 기능
│   ├── net/              # 네트워크 모듈
│   ├── obs/              # OBS 연동
│   └── sys/              # 시스템 모니터링
├── build_backend/        # C++ 빌드 결과물
├── attic/                # 이전 버전 문서 및 스크립트
└── scripts/              # 빌드 스크립트
```

### 개발 환경 설정
```bash
# 개발 의존성 설치
pip install -r requirements-dev.txt

# 코드 포맷팅
black ui_py/
isort ui_py/

# 타입 체크
mypy ui_py/

# 테스트 실행
pytest tests/
```

### 기여 가이드라인
1. **Fork** 저장소
2. **Feature branch** 생성 (`git checkout -b feature/AmazingFeature`)
3. **Commit** 변경사항 (`git commit -m 'Add some AmazingFeature'`)
4. **Push** 브랜치 (`git push origin feature/AmazingFeature`)
5. **Pull Request** 생성

### 코딩 스타일
- **Python**: PEP 8 준수, 타입 힌트 사용
- **C++**: Google C++ Style Guide 준수
- **UI**: Qt 디자인 가이드라인 준수

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 🙏 감사의 말

- **OBS Studio**: WebSocket API 제공
- **Qt/PySide6**: 크로스 플랫폼 GUI 프레임워크
- **vcpkg**: C++ 패키지 관리
- **커뮤니티**: 버그 리포트 및 기능 제안

---

**LiveOps Sentinel** - 스트리밍 품질을 한 단계 업그레이드하세요! 🚀

**개발자**: [LSH0407](https://github.com/LSH0407)
**버전**: 2.1.0
**최종 업데이트**: 2024년 12월 
