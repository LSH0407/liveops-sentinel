#!/usr/bin/env python3
"""
OBS WebSocket 연결 테스트 스크립트
"""

import asyncio
import websockets
import json
import sys
import time
import hashlib
import base64

async def test_obs_connection(host="localhost", port=4455, password=""):
    """OBS WebSocket 연결 테스트"""
    print(f"=== OBS WebSocket 연결 테스트 ===")
    print(f"호스트: {host}")
    print(f"포트: {port}")
    print(f"비밀번호: {'*' * len(password) if password else '없음'}")
    print()
    
    try:
        # 1. WebSocket 연결 시도
        print("1. WebSocket 연결 시도 중...")
        uri = f"ws://{host}:{port}"
        print(f"연결 URI: {uri}")
        
        websocket = await websockets.connect(
            uri,
            ping_interval=None,
            ping_timeout=None,
            close_timeout=5,
            max_size=2**20
        )
        print("✅ WebSocket 연결 성공!")
        
        # 2. 인증 요구사항 확인
        print("\n2. 인증 요구사항 확인 중...")
        auth_request = {
            "op": 1,  # Request
            "d": {
                "requestType": "GetAuthRequired",
                "requestId": "auth_check"
            }
        }
        
        await websocket.send(json.dumps(auth_request))
        response = await websocket.recv()
        auth_data = json.loads(response)
        
        print(f"인증 응답: {auth_data}")
        
        auth_info = auth_data.get("d", {}).get("authentication", {})
        # OBS WebSocket v5에서는 authentication 객체가 있으면 인증이 필요함
        if auth_info and (auth_info.get("authRequired", False) or "salt" in auth_info):
            print("🔐 인증이 필요함")
            
            if not password:
                print("❌ 비밀번호가 설정되지 않음")
                await websocket.close()
                return False
            
            # 3. 인증 수행
            print("\n3. 인증 수행 중...")
            
            salt = auth_info["salt"]
            challenge = auth_info["challenge"]
            print(f"Salt: {salt}")
            print(f"Challenge: {challenge}")
            
            # base64 디코딩
            salt_bytes = base64.b64decode(salt)
            challenge_bytes = base64.b64decode(challenge)
            
            # 비밀번호를 base64로 인코딩
            password_bytes = password.encode('utf-8')
            
            # secret = base64(sha256(password + salt))
            secret_input = password_bytes + salt_bytes
            secret_hash = hashlib.sha256(secret_input).digest()
            secret = base64.b64encode(secret_hash).decode('utf-8')
            
            # auth = base64(sha256(secret + challenge))
            auth_input = secret.encode('utf-8') + challenge_bytes
            auth_hash = hashlib.sha256(auth_input).digest()
            auth_response = base64.b64encode(auth_hash).decode('utf-8')
            
            # 인증 요청
            auth_request = {
                "op": 1,  # Request
                "d": {
                    "requestType": "Authenticate",
                    "requestId": "auth",
                    "authentication": auth_response
                }
            }
            
            print(f"전송할 인증 요청: {json.dumps(auth_request, indent=2)}")
            
            await websocket.send(json.dumps(auth_request))
            response = await websocket.recv()
            auth_result = json.loads(response)
            
            print(f"인증 결과: {auth_result}")
            
            if "error" in auth_result.get("d", {}):
                print("❌ 인증 실패")
                await websocket.close()
                return False
            
            print("✅ 인증 성공!")
        else:
            print("✅ 인증이 필요하지 않음")
        
        # 4. OBS 버전 정보 조회
        print("\n4. OBS 버전 정보 조회 중...")
        version_request = {
            "op": 1,  # Request
            "d": {
                "requestType": "GetVersion",
                "requestId": "version_check"
            }
        }
        
        await websocket.send(json.dumps(version_request))
        response = await websocket.recv()
        version_data = json.loads(response)
        
        print(f"OBS 버전: {version_data}")
        
        # 5. 스트림 상태 조회
        print("\n5. 스트림 상태 조회 중...")
        stream_request = {
            "op": 1,  # Request
            "d": {
                "requestType": "GetStreamStatus",
                "requestId": "stream_status"
            }
        }
        
        await websocket.send(json.dumps(stream_request))
        response = await websocket.recv()
        stream_data = json.loads(response)
        
        print(f"스트림 상태: {stream_data}")
        
        # 6. 연결 종료
        print("\n6. 연결 종료 중...")
        await websocket.close()
        
        print("\n✅ OBS WebSocket 연결 테스트 완료!")
        return True
        
    except ConnectionRefusedError:
        print("❌ 연결 거부됨 - OBS WebSocket 서버가 실행되지 않음")
        print("   OBS Studio에서 Tools > WebSocket Server Settings를 확인하세요")
        return False
        
    except Exception as e:
        if "Invalid URI" in str(e):
            print("❌ 잘못된 URI 형식")
        else:
            print(f"❌ 연결 오류: {e}")
        return False
        
    except Exception as e:
        print(f"❌ 연결 오류: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """메인 함수"""
    print("OBS WebSocket 연결 테스트")
    print("=" * 50)
    
    # 기본 설정
    host = "localhost"
    port = 4455
    password = ""
    
    # 명령행 인수 처리
    if len(sys.argv) > 1:
        host = sys.argv[1]
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    if len(sys.argv) > 3:
        password = sys.argv[3]
    
    # 테스트 실행
    result = asyncio.run(test_obs_connection(host, port, password))
    
    if result:
        print("\n🎉 OBS WebSocket 연결이 정상적으로 작동합니다!")
        sys.exit(0)
    else:
        print("\n💥 OBS WebSocket 연결에 실패했습니다.")
        print("\n확인사항:")
        print("1. OBS Studio가 실행 중인가요?")
        print("2. Tools > WebSocket Server Settings에서 서버가 활성화되어 있나요?")
        print("3. 포트 번호가 올바른가요? (기본값: 4455)")
        print("4. 비밀번호가 설정되어 있다면 올바른가요?")
        sys.exit(1)

if __name__ == "__main__":
    main()
