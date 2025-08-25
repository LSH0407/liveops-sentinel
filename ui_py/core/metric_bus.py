import json
import threading
import time
import os
from collections import deque
from typing import Callable, Dict, List, Tuple, Optional
from PySide6.QtCore import QThread, QTimer, Signal, QObject
from PySide6.QtWidgets import QApplication
from .backend_process import BackendProcess
from .obs_client import ObsClient

class MetricBus(QObject):
    """백엔드 메트릭 수집 및 브로드캐스트"""
    
    # Qt Signals
    new_metrics = Signal(dict)  # 새로운 메트릭 수신 시
    connection_lost = Signal()  # 백엔드 연결 끊김 시
    connection_established = Signal()  # 백엔드 연결 성공 시
    
    def __init__(self, backend_path: str):
        super().__init__()
        self.backend_path = backend_path
        self.backend_process: Optional[BackendProcess] = None
        self.obs_client: Optional[ObsClient] = None
        self.is_running = False
        
        # Ring buffers (60초 * 10Hz = 600 samples)
        self.buffers: Dict[str, deque] = {
            'net.rtt_ms': deque(maxlen=600),
            'net.loss_pct': deque(maxlen=600),
            'net.uplink_kbps': deque(maxlen=600),
            'sys.cpu_pct': deque(maxlen=600),
            'sys.gpu_pct': deque(maxlen=600),
            'sys.mem_mb': deque(maxlen=600),
            'obs.dropped_ratio': deque(maxlen=600),
            'obs.enc_lag_ms': deque(maxlen=600),
            'obs.render_lag_ms': deque(maxlen=600),
        }
        
        # Subscribers
        self.subscribers: List[Callable[[dict], None]] = []
        
        # Latest snapshot
        self.latest_snapshot: Optional[dict] = None
        
        # Update timer (4Hz render)
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self._broadcast_update)
        self.update_timer.start(250)  # 4Hz
        
        # OBS 클라이언트 초기화
        self._init_obs_client()
        
    def start(self):
        """백엔드 프로세스 시작"""
        if self.is_running:
            return
            
        print(f"백엔드 시작 시도: {self.backend_path}")
        
        try:
            self.backend_process = BackendProcess(self.backend_path, self)
            self.backend_process.lineReceived.connect(self._process_metrics)
            self.backend_process.readyBanner.connect(self._on_backend_ready)
            self.backend_process.errorText.connect(self._on_backend_error)
            self.backend_process.start()
            
            print(f"백엔드 프로세스 시작됨")
            self.is_running = True
            
        except Exception as e:
            print(f"백엔드 시작 실패: {e}")
            print(f"백엔드 경로: {self.backend_path}")
            print(f"경로 존재 여부: {os.path.exists(self.backend_path)}")
            self.connection_lost.emit()
    
    def stop(self):
        """백엔드 프로세스 종료"""
        self.is_running = False
        
        if self.backend_process:
            try:
                self.backend_process.stop()
            except:
                pass
            finally:
                self.backend_process = None
        
        # OBS 클라이언트 종료
        if self.obs_client:
            try:
                self.obs_client.stop()
            except:
                pass
            finally:
                self.obs_client = None
    
    def subscribe(self, callback: Callable[[dict], None]):
        """UI 컴포넌트가 메트릭 업데이트를 구독"""
        self.subscribers.append(callback)
    
    def unsubscribe(self, callback: Callable[[dict], None]):
        """구독 해제"""
        if callback in self.subscribers:
            self.subscribers.remove(callback)
    
    def latest(self) -> Optional[dict]:
        """최신 스냅샷 반환"""
        return self.latest_snapshot
    
    def series(self, key: str) -> List[Tuple[float, float]]:
        """시계열 데이터 반환 (timestamp, value)"""
        if key not in self.buffers:
            print(f"시리즈 요청: {key} - 버퍼에 없음")
            return []
        
        # Convert deque to list of tuples
        data = list(self.buffers[key])
        print(f"시리즈 요청: {key} - {len(data)}개 데이터 포인트")
        return data
    
    def send_command(self, cmd: dict):
        """백엔드에 명령 전송"""
        if not self.backend_process or not self.is_running:
            return
            
        try:
            self.backend_process.sendJson(cmd)
        except Exception as e:
            print(f"명령 전송 실패: {e}")
            self.connection_lost.emit()
    
    def _on_backend_ready(self, banner: str):
        """백엔드 레디 배너 수신"""
        print(f"백엔드 준비됨: {banner}")
        
    def _on_backend_error(self, error: str):
        """백엔드 오류 수신"""
        print(f"백엔드 오류: {error}")
        self.connection_lost.emit()
    
    def _process_metrics(self, data: dict):
        """메트릭 데이터 처리 및 버퍼 저장"""
        print(f"메트릭 수신: {data}")
        
        if 'event' not in data or data['event'] != 'metrics':
            print(f"메트릭 이벤트가 아님: {data.get('event', 'no_event')}")
            return
            
        # Extract timestamp (백엔드에서 초 단위로 오므로 그대로 사용)
        ts = data.get('ts', time.time())
        if isinstance(ts, int) and ts > 1000000000:  # Unix timestamp (초 단위)
            ts = float(ts)  # 그대로 사용
        elif isinstance(ts, int) and ts < 1000000000:  # 밀리초 단위인 경우
            ts = ts / 1000.0  # Convert ms to seconds
        
        print(f"메트릭 처리 중: CPU={data.get('cpu_pct', 0)}, GPU={data.get('gpu_pct', 0)}, RTT={data.get('rtt_ms', 0)}")
        print(f"타임스탬프: {ts}")
        
        # Store in buffers
        self._store_metric('net.rtt_ms', ts, data.get('rtt_ms', 0))
        self._store_metric('net.loss_pct', ts, data.get('loss_pct', 0))
        self._store_metric('net.uplink_kbps', ts, data.get('uplink_kbps', 0))
        
        # System metrics
        self._store_metric('sys.cpu_pct', ts, data.get('cpu_pct', 0))
        self._store_metric('sys.gpu_pct', ts, data.get('gpu_pct', 0))
        self._store_metric('sys.mem_mb', ts, data.get('mem_mb', 0))
        
        # OBS metrics (현재는 시뮬레이션 데이터)
        obs = data.get('obs', {})
        self._store_metric('obs.dropped_ratio', ts, obs.get('dropped_ratio', 0))
        self._store_metric('obs.enc_lag_ms', ts, obs.get('encoding_lag_ms', 0))
        self._store_metric('obs.render_lag_ms', ts, obs.get('render_lag_ms', 0))
        
        # Update latest snapshot
        self.latest_snapshot = data
        
        # Emit signal for immediate UI update
        print("new_metrics 시그널 발생")
        self.new_metrics.emit(data)
        
        # 메트릭 수신 시 연결 상태 업데이트
        self.connection_established.emit()
    
    def _init_obs_client(self):
        """OBS 클라이언트 초기화"""
        try:
            # 기본 설정으로 초기화 (나중에 설정에서 업데이트됨)
            self.obs_client = ObsClient()
            self.obs_client.obs_metrics_updated.connect(self._on_obs_metrics_updated)
            self.obs_client.set_metrics_callback(self._on_obs_metrics_updated)
            
            print("OBS 클라이언트 초기화 완료 (기본 설정)")
                
        except Exception as e:
            print(f"OBS 클라이언트 초기화 실패: {e}")
            import traceback
            traceback.print_exc()
    
    def _on_obs_metrics_updated(self, obs_metrics: dict):
        """OBS 메트릭 업데이트 처리"""
        try:
            ts = time.time()
            
            # OBS 메트릭을 버퍼에 저장
            self._store_metric('obs.dropped_ratio', ts, obs_metrics.get('dropped_ratio', 0))
            self._store_metric('obs.enc_lag_ms', ts, obs_metrics.get('encoding_lag_ms', 0))
            self._store_metric('obs.render_lag_ms', ts, obs_metrics.get('render_lag_ms', 0))
            
            # 최신 스냅샷에 OBS 메트릭 추가
            if self.latest_snapshot:
                if 'obs' not in self.latest_snapshot:
                    self.latest_snapshot['obs'] = {}
                self.latest_snapshot['obs'].update(obs_metrics)
            
            print(f"OBS 메트릭 업데이트: {obs_metrics}")
            
        except Exception as e:
            print(f"OBS 메트릭 처리 오류: {e}")
    
    def _store_metric(self, key: str, timestamp: float, value: float):
        """메트릭을 버퍼에 저장"""
        if key in self.buffers:
            self.buffers[key].append((timestamp, float(value)))
    
    def _broadcast_update(self):
        """구독자들에게 주기적 업데이트 브로드캐스트"""
        if not self.latest_snapshot:
            return
            
        # Create a copy for thread safety
        snapshot = dict(self.latest_snapshot)
        
        # Notify all subscribers
        for callback in self.subscribers:
            try:
                callback(snapshot)
            except Exception as e:
                print(f"구독자 콜백 오류: {e}")
    
    def get_recent_average(self, key: str, seconds: int = 5) -> Optional[float]:
        """최근 N초 평균값 계산"""
        if key not in self.buffers:
            return None
            
        now = time.time()
        recent_values = []
        
        for ts, value in self.buffers[key]:
            if now - ts <= seconds:
                recent_values.append(value)
        
        if not recent_values:
            return None
            
        return sum(recent_values) / len(recent_values)
    
    def get_latest_metrics(self) -> Optional[Dict]:
        """최신 메트릭 반환 (진단 모드 호환)"""
        if not self.latest_snapshot:
            return None
        
        # 진단 모드에서 기대하는 구조로 변환
        return {
            'net': {
                'rtt_ms': self.latest_snapshot.get('rtt_ms', 0),
                'loss_pct': self.latest_snapshot.get('loss_pct', 0),
                'uplink_kbps': self.latest_snapshot.get('uplink_kbps', 0)
            },
            'sys': {
                'cpu_pct': self.latest_snapshot.get('cpu_pct', 0),
                'gpu_pct': self.latest_snapshot.get('gpu_pct', 0),
                'memory_pct': self.latest_snapshot.get('memory_pct', 0),
                'mem_mb': self.latest_snapshot.get('mem_mb', 0)
            },
            'obs': self.latest_snapshot.get('obs', {})
        }
    
    def update_obs_metrics(self, obs_metrics: Dict):
        """OBS 메트릭 업데이트"""
        print(f"=== 메트릭 버스 OBS 업데이트 ===")
        print(f"받은 OBS 메트릭: {obs_metrics}")
        print(f"현재 latest_snapshot: {self.latest_snapshot}")
        
        if self.latest_snapshot:
            if 'obs' not in self.latest_snapshot:
                self.latest_snapshot['obs'] = {}
            
            self.latest_snapshot['obs'].update(obs_metrics)
            
            # 메트릭 저장
            ts = time.time()
            dropped = obs_metrics.get('dropped_ratio', 0)
            enc_lag = obs_metrics.get('encoding_lag_ms', 0)
            render_lag = obs_metrics.get('render_lag_ms', 0)
            
            self._store_metric('obs.dropped_ratio', ts, dropped)
            self._store_metric('obs.encoding_lag_ms', ts, enc_lag)
            self._store_metric('obs.render_lag_ms', ts, render_lag)
            
            print(f"OBS 메트릭 저장됨: dropped={dropped}, enc_lag={enc_lag}, render_lag={render_lag}")
            print(f"업데이트된 latest_snapshot: {self.latest_snapshot}")
        else:
            print("latest_snapshot이 None이므로 OBS 메트릭 저장 불가")
        
        print(f"=== 메트릭 버스 OBS 업데이트 끝 ===")
    
    def reconfigure_obs_client(self, host: str, port: int, password: str = ""):
        """OBS 클라이언트 재설정"""
        print(f"=== OBS 클라이언트 재설정 ===")
        print(f"새 설정: host={host}, port={port}, password={'*' * len(password) if password else '없음'}")
        
        try:
            # 기존 OBS 클라이언트 중지
            if self.obs_client:
                print("기존 OBS 클라이언트 중지")
                self.obs_client.stop()
            
            # 새로운 설정으로 OBS 클라이언트 생성
            self.obs_client = ObsClient(host=host, port=port, password=password)
            self.obs_client.obs_metrics_updated.connect(self._on_obs_metrics_updated)
            self.obs_client.set_metrics_callback(self._on_obs_metrics_updated)
            
            print("새 OBS 클라이언트 생성 완료")
            
            # OBS 연결 시도
            print("OBS 연결 시도 중...")
            self.obs_client.start()
            print("OBS 연결 시도 완료 - 연결 상태 확인 중...")
            
        except Exception as e:
            print(f"OBS 클라이언트 재설정 실패: {e}")
            import traceback
            traceback.print_exc()
