"""
OBS WebSocket v5 폴링 클라이언트
"""

import threading
import time
from typing import Dict, Optional
from PySide6.QtCore import QObject, Signal

try:
    from obsws_python import ReqClient, error as obs_err
    OBS_AVAILABLE = True
except ImportError:
    OBS_AVAILABLE = False
    print("⚠️ obsws-python이 설치되지 않았습니다. pip install obsws-python>=1.6.0")

class ObsPoller(QObject):
    """OBS WebSocket v5 폴링 클라이언트"""
    
    # Qt Signals
    tick = Signal(dict)  # OBS 메트릭 업데이트
    connected = Signal()  # 연결 성공
    disconnected = Signal()  # 연결 끊김
    
    def __init__(self, host: str = "127.0.0.1", port: int = 4455, password: str = "", use_tls: bool = False):
        super().__init__()
        self.host = host
        self.port = port
        self.password = (password or "").strip()
        self.use_tls = use_tls
        self._client = None
        self.is_running = False
        self.is_connected = False
        self.thread = None
        
        # 메트릭 캐시
        self.latest_metrics = {
            'dropped_ratio': 0.0,
            'encoding_lag_ms': 0.0,
            'render_lag_ms': 0.0,
            'fps': 0.0,
            'bitrate_kbps': 0.0
        }
        
        # OBS 설정 캐시
        self.obs_settings = {
            'output_resolution': 'Unknown',
            'fps': 'Unknown',
            'bitrate': 'Unknown',
            'encoder': 'Unknown'
        }
    
    def connect(self) -> bool:
        """OBS 연결 시도"""
        if not OBS_AVAILABLE:
            print("❌ obsws-python이 설치되지 않음")
            return False
        
        try:
            print(f"=== OBS Poller 연결 시도 ===")
            print(f"호스트: {self.host}")
            print(f"포트: {self.port}")
            print(f"TLS: {self.use_tls}")
            print(f"비밀번호 길이: {len(self.password) if self.password else 0}")
            
            # TLS 설정에 따른 연결
            if self.use_tls:
                self._client = ReqClient(
                    host=self.host, 
                    port=self.port, 
                    password=self.password,
                    timeout=3.0, 
                    url=f"wss://{self.host}"
                )
            else:
                self._client = ReqClient(
                    host=self.host, 
                    port=self.port, 
                    password=self.password,
                    timeout=3.0
                )
            
            # 연결 테스트
            version = self._client.get_version()
            print(f"✅ OBS Poller 연결 성공: {version.obs_web_socket_version}")
            
            self.is_connected = True
            self.connected.emit()
            return True
            
        except obs_err.OBSSDKError as e:
            if "authentication enabled but no password provided" in str(e):
                print(f"❌ OBS 인증 실패: {e}")
                print("   OBS Studio에서 인증이 활성화되어 있지만 비밀번호가 제공되지 않음")
            else:
                print(f"❌ OBS SDK 오류: {e}")
            return False
        except Exception as e:
            print(f"❌ OBS Poller 연결 예외: {type(e).__name__}: {e}")
            return False
    
    def disconnect(self):
        """OBS 연결 해제"""
        if self._client:
            try:
                self._client.disconnect()
            except:
                pass
            finally:
                self._client = None
        
        self.is_connected = False
        self.disconnected.emit()
    
    def start_polling(self):
        """폴링 시작"""
        if self.is_running:
            return
        
        self.is_running = True
        self.thread = threading.Thread(target=self._polling_loop, daemon=True)
        self.thread.start()
        print("OBS Poller 시작됨")
    
    def stop_polling(self):
        """폴링 중지"""
        self.is_running = False
        if self.thread:
            self.thread.join(timeout=2.0)
        self.disconnect()
        print("OBS Poller 중지됨")
    
    def _polling_loop(self):
        """폴링 루프"""
        print("OBS Polling 루프 시작")
        
        try:
            # 연결 시도
            if not self.connect():
                print("OBS Poller 연결 실패")
                return
            
            print("OBS Poller 연결 성공, 폴링 루프 시작")
            
            # 폴링 루프
            while self.is_running and self.is_connected:
                try:
                    # OBS 통계 조회
                    stats = self._client.get_stats()
                    
                    # 메트릭 계산
                    # 프레임 드랍 비율: 출력에서 스킵된 프레임 비율 (0.0~1.0)
                    dropped_ratio = 0.0
                    if hasattr(stats, 'output_skipped_frames') and hasattr(stats, 'output_total_frames'):
                        if stats.output_total_frames > 0:
                            dropped_ratio = stats.output_skipped_frames / stats.output_total_frames

                    # 스트림 상태 조회 (먼저 수행)
                    stream_stats = self._client.get_stream_status()
                    
                    # 인코딩/렌더 지연(ms): 스트림 상태에서 우선 취득, 없으면 평균 렌더 시간으로 대체
                    encoding_lag_ms = 0.0
                    render_lag_ms = 0.0
                    if hasattr(stream_stats, 'encoding_lag') and stream_stats.encoding_lag is not None:
                        encoding_lag_ms = float(stream_stats.encoding_lag)
                    if hasattr(stream_stats, 'render_lag') and stream_stats.render_lag is not None:
                        render_lag_ms = float(stream_stats.render_lag)
                    # fallback
                    if render_lag_ms == 0.0 and hasattr(stats, 'average_frame_render_time') and stats.average_frame_render_time is not None:
                        render_lag_ms = float(stats.average_frame_render_time)
                    fps = stream_stats.fps if hasattr(stream_stats, 'fps') else 0.0
                    bitrate = stream_stats.bitrate / 1000.0 if hasattr(stream_stats, 'bitrate') and stream_stats.bitrate else 0.0
                    
                    # 메트릭 업데이트
                    self.latest_metrics.update({
                        'dropped_ratio': dropped_ratio,
                        'encoding_lag_ms': encoding_lag_ms,
                        'render_lag_ms': render_lag_ms,
                        'fps': fps,
                        'bitrate_kbps': bitrate
                    })
                    
                    print(f"OBS Poller 메트릭: dropped={dropped_ratio:.3f}, enc_ms={encoding_lag_ms:.2f}ms, render_ms={render_lag_ms:.2f}ms, fps={fps}, bitrate={bitrate:.1f}kbps")
                    
                    # 시그널 발생
                    self.tick.emit(self.latest_metrics.copy())
                    
                except Exception as e:
                    print(f"OBS Poller 메트릭 조회 오류: {e}")
                    # 연결 끊김으로 처리
                    self.is_connected = False
                    self.disconnected.emit()
                    break
                
                time.sleep(1)  # 1초 간격 폴링
                
        except Exception as e:
            print(f"OBS Poller 루프 오류: {e}")
            import traceback
            traceback.print_exc()
        finally:
            print("OBS Poller 루프 종료")
            self.disconnect()
    
    def get_latest_metrics(self) -> Dict:
        """최신 메트릭 반환"""
        return self.latest_metrics.copy()
    
    def get_obs_settings(self) -> Dict:
        """OBS 출력 설정 조회"""
        if not self.is_connected or not self._client:
            print("OBS 설정 조회 실패: 연결되지 않음")
            return self.obs_settings.copy()
        
        try:
            print("=== OBS 설정 조회 시작 ===")
            
            # 비디오 설정 조회
            video_settings = self._client.get_video_settings()
            print(f"비디오 설정: {video_settings}")
            
            # 설정 파싱 - 더 간단한 방법
            resolution = "Unknown"
            fps = "Unknown"
            bitrate = "Unknown"
            encoder = "Unknown"
            
            try:
                # 해상도 조회 - 직접 속성 접근
                width = getattr(video_settings, 'output_width', None)
                height = getattr(video_settings, 'output_height', None)
                print(f"해상도 원시값: width={width}, height={height}")
                
                if width and height:
                    resolution = f"{width}x{height}"
                    print(f"해상도 파싱 성공: {resolution}")
                
                # FPS 조회 - 직접 속성 접근
                fps_num = getattr(video_settings, 'fps_num', None)
                fps_den = getattr(video_settings, 'fps_den', None)
                print(f"FPS 원시값: fps_num={fps_num}, fps_den={fps_den}")
                
                if fps_num and fps_den:
                    fps_value = fps_num / fps_den
                    fps = f"{fps_num}/{fps_den} ({fps_value:.1f})"
                    print(f"FPS 파싱 성공: {fps}")
            except Exception as e:
                print(f"비디오 설정 파싱 오류: {e}")
            
            try:
                # 스트림 서비스 설정에서 비트레이트 조회
                stream_settings = self._client.get_stream_service_settings()
                print(f"스트림 설정: {stream_settings}")
                
                # settings 속성에서 비트레이트 찾기
                if hasattr(stream_settings, 'settings') and stream_settings.settings:
                    settings = stream_settings.settings
                    print(f"스트림 설정 내부: {settings}")
                    if isinstance(settings, dict) and 'bitrate' in settings:
                        bitrate = f"{settings['bitrate']} kbps"
                        print(f"비트레이트 파싱 성공: {bitrate}")
            except Exception as e:
                print(f"비트레이트 조회 오류: {e}")
            
            try:
                # 출력 설정에서 인코더 조회
                output_settings = self._client.get_output_settings()
                print(f"출력 설정: {output_settings}")
                
                # output_settings 속성에서 인코더 찾기
                if hasattr(output_settings, 'output_settings') and output_settings.output_settings:
                    settings = output_settings.output_settings
                    print(f"출력 설정 내부: {settings}")
                    if isinstance(settings, dict) and 'encoder' in settings:
                        encoder = settings['encoder']
                        print(f"인코더 파싱 성공: {encoder}")
            except Exception as e:
                print(f"인코더 조회 오류: {e}")
            
            # 설정 업데이트
            self.obs_settings.update({
                'output_resolution': resolution,
                'fps': fps,
                'bitrate': bitrate,
                'encoder': encoder
            })
            
            print(f"OBS 설정 조회 완료: {resolution}, {fps}, {bitrate}, {encoder}")
            
        except Exception as e:
            print(f"OBS 설정 조회 오류: {e}")
            import traceback
            traceback.print_exc()
        
        return self.obs_settings.copy()
