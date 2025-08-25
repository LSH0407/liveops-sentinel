# OBS WebSocket 연결 가이드

## 1. OBS Studio WebSocket Server 설정

### 1.1 OBS Studio 실행
- OBS Studio를 실행합니다.

### 1.2 WebSocket Server Settings 열기
- **Tools** → **WebSocket Server Settings** 클릭

### 1.3 서버 설정
- **Enable WebSocket server** 체크박스 활성화
- **Server Port**: `4455` (기본값)
- **Enable Authentication**: 체크박스 활성화
- **Server Password**: 원하는 비밀번호 설정 (예: `mypassword123`)
- **Apply** 클릭

### 1.4 설정 확인
- **OK** 클릭하여 설정 저장

## 2. LiveOps Sentinel에서 OBS 설정

### 2.1 GUI 실행
- `LiveOpsSentinel.exe` 실행

### 2.2 설정 메뉴 열기
- 상단 **설정** 버튼 클릭

### 2.3 OBS 연동 탭
- **OBS 연동** 탭 선택

### 2.4 연결 정보 입력
- **서버 IP**: `localhost` (또는 OBS가 실행된 컴퓨터의 IP)
- **포트**: `4455`
- **비밀번호**: OBS에서 설정한 비밀번호 입력

### 2.5 설정 저장
- **확인** 클릭

## 3. 연결 테스트

### 3.1 콘솔 로그 확인
GUI 실행 시 콘솔에서 다음 로그 확인:
```
=== OBS 클라이언트 재설정 ===
새 설정: host=localhost, port=4455, password=****
기존 OBS 클라이언트 중지
새 OBS 클라이언트 생성 완료
OBS 감지됨 - 연결 시도
OBS WebSocket 연결 시도: ws://localhost:4455
OBS WebSocket 연결됨, 인증 확인 중...
OBS 인증 시도 (비밀번호 길이: 12)
OBS 인증이 필요함
OBS 인증 정보: salt=..., challenge=...
OBS 인증 해시 생성 완료
OBS 인증 결과: {...}
OBS 인증 성공
OBS WebSocket 연결 성공
```

### 3.2 GUI 상태 확인
- **OBS 연결 상태**: "연결됨" 표시
- **OBS 메트릭**: 드롭된 프레임, 인코딩 지연, 렌더링 지연 값 업데이트

## 4. 문제 해결

### 4.1 연결 거부 오류
- OBS Studio가 실행 중인지 확인
- WebSocket Server가 활성화되어 있는지 확인
- 포트 번호가 올바른지 확인

### 4.2 인증 실패 오류
- OBS에서 설정한 비밀번호가 정확한지 확인
- 비밀번호에 특수문자가 포함된 경우 이스케이프 처리

### 4.3 메트릭이 업데이트되지 않음
- OBS에서 스트림을 시작했는지 확인
- 스트림 상태가 "Live"인지 확인

## 5. 테스트 스크립트 사용

비밀번호가 설정된 경우:
```bash
python test_obs_connection.py localhost 4455 your_password
```

비밀번호가 없는 경우:
```bash
python test_obs_connection.py localhost 4455
```
