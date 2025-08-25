# OBS WebSocket 비밀번호 확인 가이드

## 현재 상황
- OBS WebSocket 연결 시도 성공 ✅
- OBS에서 인증 요구사항 확인 ✅
- 인증 실패 ❌

## 해결 방법

### 1. OBS Studio에서 현재 비밀번호 확인
1. **OBS Studio 실행**
2. **Tools** → **WebSocket Server Settings** 클릭
3. **Server Password** 필드에 설정된 비밀번호 확인
4. **비밀번호를 복사**하여 메모장에 임시 저장

### 2. LiveOps Sentinel에서 비밀번호 재설정
1. **LiveOps Sentinel GUI 실행**
2. **설정** 버튼 클릭
3. **OBS 연동** 탭 선택
4. **비밀번호 필드 비우기** (기존 값 삭제)
5. **OBS에서 복사한 비밀번호 정확히 입력**
6. **확인** 클릭

### 3. 연결 테스트
1. **콘솔 로그 확인**:
   ```
   === OBS 클라이언트 재설정 ===
   새 설정: host=192.168.0.12, port=4455, password=****
   OBS WebSocket 연결 시도: ws://192.168.0.12:4455
   OBS WebSocket 연결됨, 인증 확인 중...
   OBS 인증 시도 (비밀번호 길이: X)
   OBS 인증이 필요함
   OBS 인증 정보: salt=..., challenge=...
   OBS 인증 해시 생성 완료
   OBS 인증 결과: {...}
   ✅ OBS 인증 성공
   ```

### 4. 문제가 지속되는 경우

#### 4.1 OBS에서 인증 비활성화 (임시 해결)
1. **OBS Studio** → **Tools** → **WebSocket Server Settings**
2. **Enable Authentication** 체크박스 **해제**
3. **Apply** → **OK**
4. **LiveOps Sentinel에서 비밀번호 필드 비우기**

#### 4.2 새로운 비밀번호 설정
1. **OBS Studio** → **Tools** → **WebSocket Server Settings**
2. **Server Password** 필드에 **새로운 간단한 비밀번호** 입력 (예: `123456`)
3. **Apply** → **OK**
4. **LiveOps Sentinel에서 새 비밀번호 입력**

### 5. 테스트 스크립트로 확인
```bash
# 비밀번호 없이 테스트
python test_obs_connection.py 192.168.0.12 4455

# 비밀번호와 함께 테스트
python test_obs_connection.py 192.168.0.12 4455 your_password
```

## 주의사항
- 비밀번호에 **특수문자**가 포함된 경우 정확히 입력
- **대소문자** 구분 확인
- **공백** 문자가 포함되지 않았는지 확인
- **복사-붙여넣기** 시 추가 문자가 들어가지 않았는지 확인
