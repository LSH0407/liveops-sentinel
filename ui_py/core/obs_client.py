"""
OBS Studio WebSocket 클라이언트 (obsws-python v5)
"""

import threading
import time
from typing import Dict, Optional, Callable, Tuple
from PySide6.QtCore import QObject, Signal

try:
    from obsws_python import ReqClient, error as obs_err
    OBS_AVAILABLE = True
except ImportError:
    OBS_AVAILABLE = False
    print("⚠️ obsws-python이 설치되지 않았습니다. pip install obsws-python>=1.6.0")

class ObsClient(QObject):
    """OBS WebSocket v5 클라이언트"""
    
    # Qt Signals
    obs_connected = Signal()
    obs_disconnected = Signal()
    obs_metrics_updated = Signal(dict)
    
    def __init__(self, host: str = "127.0.0.1", port: int = 4455, password: str = "", use_tls: bool = False, timeout: float = 3.0):
        super().__init__()
        self.host = host
        self.port = port
        self.password = (password or "").strip()
        self.use_tls = use_tls
        self.timeout = timeout
        self._cli = None
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
    
    def connect(self) -> bool:
        """OBS 연결 시도"""
        if not OBS_AVAILABLE:
            print("❌ obsws-python이 설치되지 않음")
            return False
            
        try:
            print(f"=== OBS WebSocket v5 연결 시도 ===")
            print(f"호스트: {self.host}")
            print(f"포트: {self.port}")
            print(f"TLS: {self.use_tls}")
            print(f"비밀번호 길이: {len(self.password) if self.password else 0}")
            
            # TLS 설정에 따른 연결
            if self.use_tls:
                self._cli = ReqClient(
                    host=self.host, 
                    port=self.port, 
                    password=self.password,
                    timeout=self.timeout, 
                    url=f"wss://{self.host}"
                )
            else:
                self._cli = ReqClient(
                    host=self.host, 
                    port=self.port, 
                    password=self.password,
                    timeout=self.timeout
                )
            
            # 연결 테스트
            version = self._cli.get_version()
            print(f"✅ OBS 연결 성공: {version.obs_web_socket_version}")
            
            self.is_connected = True
            self.obs_connected.emit()
            return True
            
        except obs_err.OBSSDKError as e:
            if "authentication enabled but no password provided" in str(e):
                print(f"❌ OBS 인증 실패: {e}")
                print("   OBS Studio에서 인증이 활성화되어 있지만 비밀번호가 제공되지 않음")
                print("   OBS Studio에서 Tools > WebSocket Server Settings에서 인증을 비활성화하거나")
                print("   비밀번호를 설정하고 LiveOps Sentinel 설정에 입력하세요")
            else:
                print(f"❌ OBS SDK 오류: {e}")
            return False
        except Exception as e:
            print(f"❌ OBS 연결 예외: {type(e).__name__}: {e}")
            return False
    
    def test_connect(self) -> Tuple[bool, str]:
        """연결 테스트 - 성공/실패와 메시지 반환"""
        if not OBS_AVAILABLE:
            return False, "obsws-python이 설치되지 않음"
            
        try:
            version = self.connect()
            if version:
                return True, f"OBS 연결 OK: {self._cli.get_version().obs_web_socket_version}"
            else:
                return False, "연결 실패"
        except obs_err.OBSSDKError as e:
            if "authentication enabled but no password provided" in str(e):
                return False, "인증 실패: OBS Studio에서 인증이 활성화되어 있지만 비밀번호가 제공되지 않음"
            else:
                return False, f"OBS SDK 오류: {e}"
        except Exception as e:
            return False, f"예외: {type(e).__name__}: {e}"
    
    def start(self):
        """OBS 연결 시작"""
        if self.is_running:
            return
            
        self.is_running = True
        self.thread = threading.Thread(target=self._run_metrics_loop, daemon=True)
        self.thread.start()
    
    def stop(self):
        """OBS 연결 중지"""
        self.is_running = False
        if self._cli:
            try:
                self._cli.disconnect()
            except:
                pass
            self._cli = None
        self.is_connected = False
        self.obs_disconnected.emit()
    
    def set_metrics_callback(self, callback: Callable[[dict], None]):
        """메트릭 업데이트 콜백 설정"""
        self.metrics_callback = callback
    
    def get_latest_metrics(self) -> Dict:
        """최신 OBS 메트릭 반환"""
        return self.latest_metrics.copy()
    
    def client(self):
        """OBS 클라이언트 반환"""
        return self._cli
    
    def _run_metrics_loop(self):
        """메트릭 수집 루프"""
        print("OBS 메트릭 루프 시작")
        
        try:
            # 연결 시도
            if not self.connect():
                print("OBS 연결 실패")
                self.is_connected = False
                self.obs_disconnected.emit()
                return
            
            # 메트릭 수집 루프
            while self.is_running and self.is_connected:
                try:
                    # 스트림 상태 조회
                    stats = self._cli.get_stream_status()
                    
                    # 메트릭 업데이트
                    self.latest_metrics.update({
                        'dropped_ratio': stats.dropped_frames / max(stats.total_frames, 1) if stats.total_frames > 0 else 0.0,
                        'encoding_lag_ms': stats.encoding_lag,
                        'render_lag_ms': stats.render_lag,
                        'fps': stats.fps,
                        'bitrate_kbps': stats.bitrate / 1000.0 if stats.bitrate else 0.0
                    })
                    
                    # 콜백 호출
                    if self.metrics_callback:
                        self.metrics_callback(self.latest_metrics)
                    
                    # 시그널 발생
                    self.obs_metrics_updated.emit(self.latest_metrics)
                    
                except Exception as e:
                    print(f"OBS 메트릭 조회 오류: {e}")
                    self.is_connected = False
                    self.obs_disconnected.emit()
                    break
                
                time.sleep(1)  # 1초 간격
                
        except Exception as e:
            print(f"OBS 메트릭 루프 오류: {e}")
            self.is_connected = False
            self.obs_disconnected.emit()
        finally:
            print("OBS 메트릭 루프 종료")
            if self._cli:
                try:
                    self._cli.disconnect()
                except:
                    pass
                self._cli = None
            self.is_connected = False
            self.obs_disconnected.emit()
    
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
