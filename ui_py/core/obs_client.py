"""
OBS Studio WebSocket 클라이언트 (PyInstaller 호환 버전)
"""

import asyncio
import websockets
import json
import threading
import time
from typing import Dict, Optional, Callable
from PySide6.QtCore import QObject, Signal

class ObsClient(QObject):
    """OBS WebSocket 클라이언트"""
    
    # Qt Signals
    obs_connected = Signal()
    obs_disconnected = Signal()
    obs_metrics_updated = Signal(dict)
    
    def __init__(self, host: str = "localhost", port: int = 4455, password: str = ""):
        super().__init__()
        self.host = host
        self.port = port
        self.password = password
        self.websocket = None
        self.is_connected = False
        self.is_running = False
        self.thread = None
        
        # OBS 메트릭 캐시
        self.latest_metrics = {
            'dropped_ratio': 0.0,
            'encoding_lag_ms': 0.0,
            'render_lag_ms': 0.0,
            'fps': 0.0,
            'bitrate_kbps': 0.0
        }
        
        # 메트릭 업데이트 콜백
        self.metrics_callback: Optional[Callable[[dict], None]] = None
    
    def start(self):
        """OBS 연결 시작"""
        if self.is_running:
            return
            
        self.is_running = True
        self.thread = threading.Thread(target=self._run_websocket_loop, daemon=True)
        self.thread.start()
    
    def stop(self):
        """OBS 연결 중지"""
        self.is_running = False
        if self.websocket:
            asyncio.run(self._close_websocket())
    
    def set_metrics_callback(self, callback: Callable[[dict], None]):
        """메트릭 업데이트 콜백 설정"""
        self.metrics_callback = callback
    
    def get_latest_metrics(self) -> Dict:
        """최신 OBS 메트릭 반환"""
        return self.latest_metrics.copy()
    
    async def _connect_websocket(self):
        """WebSocket 연결"""
        try:
            print(f"=== OBS WebSocket 연결 시도 ===")
            print(f"호스트: {self.host}")
            print(f"포트: {self.port}")
            print(f"비밀번호 길이: {len(self.password) if self.password else 0}")
            print(f"OBS WebSocket 연결 시도: ws://{self.host}:{self.port}")
            uri = f"ws://{self.host}:{self.port}"
            
            # 연결 옵션 개선
            self.websocket = await websockets.connect(
                uri, 
                ping_interval=None, 
                ping_timeout=None,
                close_timeout=5,
                max_size=2**20  # 1MB
            )
            
            print("OBS WebSocket 연결됨, 인증 확인 중...")
            
            # 인증 시도 (OBS WebSocket v5에서는 항상 인증 확인 필요)
            print(f"OBS 인증 시도 (비밀번호 길이: {len(self.password) if self.password else 0})")
            auth_response = await self._authenticate()
            if not auth_response:
                print("OBS 인증 실패")
                return False
            print("OBS 인증 성공")
            
            # 이벤트 구독
            await self._subscribe_to_events()
            
            self.is_connected = True
            self.obs_connected.emit()
            print("OBS WebSocket 연결 성공")
            return True
            
        except Exception as e:
            print(f"OBS WebSocket 연결 실패: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    async def _authenticate(self) -> bool:
        """OBS 인증"""
        try:
            print("OBS 인증 요구사항 확인 중...")
            auth_request = {
                "op": 1,  # Request
                "d": {
                    "requestType": "GetAuthRequired",
                    "requestId": "auth_check"
                }
            }
            
            await self.websocket.send(json.dumps(auth_request))
            response = await self.websocket.recv()
            auth_data = json.loads(response)
            
            print(f"OBS 인증 응답: {auth_data}")
            
            auth_info = auth_data.get("d", {}).get("authentication", {})
            # OBS WebSocket v5에서는 authentication 객체가 있으면 인증이 필요함
            if auth_info and (auth_info.get("authRequired", False) or "salt" in auth_info):
                print("OBS 인증이 필요함")
                
                if not self.password:
                    print("❌ OBS 인증이 필요하지만 비밀번호가 설정되지 않음")
                    print("   OBS Studio에서 Tools > WebSocket Server Settings에서 비밀번호를 설정하거나")
                    print("   LiveOps Sentinel 설정에서 OBS 비밀번호를 입력해주세요")
                    return False
                
                # 비밀번호 해시 계산 (간단한 구현)
                import hashlib
                import base64
                
                salt = auth_info["salt"]
                challenge = auth_info["challenge"]
                
                print(f"OBS 인증 정보: salt={salt}, challenge={challenge}")
                
                # base64 디코딩
                salt_bytes = base64.b64decode(salt)
                challenge_bytes = base64.b64decode(challenge)
                
                # 비밀번호를 base64로 인코딩
                password_bytes = self.password.encode('utf-8')
                
                # secret = base64(sha256(password + salt))
                secret_input = password_bytes + salt_bytes
                secret_hash = hashlib.sha256(secret_input).digest()
                secret = base64.b64encode(secret_hash).decode('utf-8')
                
                # auth = base64(sha256(secret + challenge))
                auth_input = secret.encode('utf-8') + challenge_bytes
                auth_hash = hashlib.sha256(auth_input).digest()
                auth_response = base64.b64encode(auth_hash).decode('utf-8')
                
                print(f"OBS 인증 해시 생성 완료")
                
                auth_request = {
                    "op": 1,  # Request
                    "d": {
                        "requestType": "Authenticate",
                        "requestId": "auth",
                        "authentication": auth_response
                    }
                }
                
                await self.websocket.send(json.dumps(auth_request))
                response = await self.websocket.recv()
                auth_result = json.loads(response)
                
                print(f"OBS 인증 결과: {auth_result}")
                
                if "error" in auth_result.get("d", {}):
                    print(f"❌ OBS 인증 실패: {auth_result.get('d', {}).get('error', 'Unknown error')}")
                    return False
                
                print("✅ OBS 인증 성공")
                return True
            else:
                print("✅ OBS 인증이 필요하지 않음")
                return True
            
        except Exception as e:
            print(f"OBS 인증 오류: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    async def _subscribe_to_events(self):
        """OBS 이벤트 구독"""
        try:
            # 스트림 상태 이벤트 구독
            subscribe_request = {
                "op": 1,  # Request
                "d": {
                    "requestType": "SetHeartbeat",
                    "requestId": "heartbeat",
                    "enable": True
                }
            }
            
            await self.websocket.send(json.dumps(subscribe_request))
            
        except Exception as e:
            print(f"OBS 이벤트 구독 오류: {e}")
    
    async def _get_stream_stats(self):
        """스트림 통계 조회"""
        try:
            stats_request = {
                "op": 1,  # Request
                "d": {
                    "requestType": "GetStreamStatus",
                    "requestId": "stream_stats"
                }
            }
            
            await self.websocket.send(json.dumps(stats_request))
            response = await self.websocket.recv()
            stats_data = json.loads(response)
            
            if "error" not in stats_data.get("d", {}):
                stream_data = stats_data.get("d", {}).get("responseData", {})
                
                # 메트릭 업데이트
                self.latest_metrics.update({
                    'dropped_ratio': stream_data.get("droppedFrames", 0) / max(stream_data.get("totalFrames", 1), 1),
                    'encoding_lag_ms': stream_data.get("encodingLag", 0),
                    'render_lag_ms': stream_data.get("renderLag", 0),
                    'fps': stream_data.get("fps", 0),
                    'bitrate_kbps': stream_data.get("bitrate", 0) / 1000.0
                })
                
                # 콜백 호출
                if self.metrics_callback:
                    self.metrics_callback(self.latest_metrics)
                
                # 시그널 발생
                self.obs_metrics_updated.emit(self.latest_metrics)
                
        except Exception as e:
            print(f"OBS 스트림 통계 조회 오류: {e}")
    
    async def _close_websocket(self):
        """WebSocket 연결 종료"""
        if self.websocket:
            await self.websocket.close()
            self.websocket = None
            self.is_connected = False
            self.obs_disconnected.emit()
    
    def _run_websocket_loop(self):
        """WebSocket 메인 루프"""
        print("OBS WebSocket 루프 시작")
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        
        try:
            # 연결 시도
            print("OBS WebSocket 연결 시도 중...")
            if loop.run_until_complete(self._connect_websocket()):
                print("OBS WebSocket 연결 성공, 메인 루프 시작")
                # 메인 루프
                while self.is_running and self.is_connected:
                    try:
                        # 스트림 통계 조회
                        loop.run_until_complete(self._get_stream_stats())
                        
                        # 메시지 수신
                        if self.websocket:
                            try:
                                message = loop.run_until_complete(
                                    asyncio.wait_for(self.websocket.recv(), timeout=1.0)
                                )
                                self._handle_message(json.loads(message))
                            except asyncio.TimeoutError:
                                pass  # 타임아웃은 정상
                            except websockets.exceptions.ConnectionClosed:
                                print("OBS WebSocket 연결이 끊어짐")
                                self.is_connected = False
                                self.obs_disconnected.emit()
                                break
                            except Exception as e:
                                print(f"OBS 메시지 수신 오류: {e}")
                                if "Connection closed" in str(e):
                                    self.is_connected = False
                                    self.obs_disconnected.emit()
                                    break
                        
                        time.sleep(1)  # 1초 간격
                        
                    except Exception as e:
                        print(f"OBS WebSocket 루프 오류: {e}")
                        import traceback
                        traceback.print_exc()
                        self.is_connected = False
                        self.obs_disconnected.emit()
                        break
            else:
                print("OBS WebSocket 연결 실패")
                self.is_connected = False
                self.obs_disconnected.emit()
        except Exception as e:
            print(f"OBS WebSocket 루프 초기화 오류: {e}")
            import traceback
            traceback.print_exc()
            self.is_connected = False
            self.obs_disconnected.emit()
        finally:
            print("OBS WebSocket 루프 종료")
            if self.is_connected:
                loop.run_until_complete(self._close_websocket())
            loop.close()
    
    def _handle_message(self, message: dict):
        """OBS 메시지 처리"""
        try:
            if message.get("op") == 0 and message.get("d", {}).get("eventType") == "StreamStatus":
                # 스트림 상태 업데이트
                stream_data = message.get("d", {}).get("eventData", {})
                
                self.latest_metrics.update({
                    'dropped_ratio': stream_data.get("droppedFrames", 0) / max(stream_data.get("totalFrames", 1), 1),
                    'encoding_lag_ms': stream_data.get("encodingLag", 0),
                    'render_lag_ms': stream_data.get("renderLag", 0),
                    'fps': stream_data.get("fps", 0),
                    'bitrate_kbps': stream_data.get("bitrate", 0) / 1000.0
                })
                
                # 콜백 호출
                if self.metrics_callback:
                    self.metrics_callback(self.latest_metrics)
                
                # 시그널 발생
                self.obs_metrics_updated.emit(self.latest_metrics)
                
        except Exception as e:
            print(f"OBS 메시지 처리 오류: {e}")
    
    def is_obs_running(self) -> bool:
        """OBS가 실행 중인지 확인"""
        try:
            import psutil
            for proc in psutil.process_iter(['name']):
                if proc.info['name'] and 'obs' in proc.info['name'].lower():
                    return True
            return False
        except:
            return False
