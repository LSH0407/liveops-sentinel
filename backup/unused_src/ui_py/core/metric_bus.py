import json
import threading
import time
from collections import deque
from typing import Callable, Dict, List, Tuple, Optional
from PySide6.QtCore import QThread, QTimer, Signal, QObject
from PySide6.QtWidgets import QApplication
import subprocess

class MetricBus(QObject):
    """백엔드 메트릭 수집 및 브로드캐스트"""
    
    # Qt Signals
    new_metrics = Signal(dict)  # 새로운 메트릭 수신 시
    connection_lost = Signal()  # 백엔드 연결 끊김 시
    
    def __init__(self, backend_path: str):
        super().__init__()
        self.backend_path = backend_path
        self.backend_process: Optional[subprocess.Popen] = None
        self.reader_thread: Optional[QThread] = None
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
        
    def start(self):
        """백엔드 프로세스 시작"""
        if self.is_running:
            return
            
        try:
            self.backend_process = subprocess.Popen(
                [self.backend_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            self.is_running = True
            self.reader_thread = threading.Thread(target=self._read_loop, daemon=True)
            self.reader_thread.start()
            
        except Exception as e:
            print(f"백엔드 시작 실패: {e}")
            self.connection_lost.emit()
    
    def stop(self):
        """백엔드 프로세스 종료"""
        self.is_running = False
        
        if self.backend_process:
            try:
                self.backend_process.terminate()
                self.backend_process.wait(timeout=3)
            except:
                self.backend_process.kill()
            finally:
                self.backend_process = None
    
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
            return []
        
        # Convert deque to list of tuples
        return list(self.buffers[key])
    
    def send_command(self, cmd: dict):
        """백엔드에 명령 전송"""
        if not self.backend_process or not self.is_running:
            return
            
        try:
            line = json.dumps(cmd, ensure_ascii=False)
            self.backend_process.stdin.write(line + "\n")
            self.backend_process.stdin.flush()
        except Exception as e:
            print(f"명령 전송 실패: {e}")
            self.connection_lost.emit()
    
    def _read_loop(self):
        """백엔드 stdout 읽기 루프"""
        while self.is_running and self.backend_process and self.backend_process.stdout:
            try:
                line = self.backend_process.stdout.readline()
                if not line:
                    break
                    
                data = json.loads(line.strip())
                self._process_metrics(data)
                
            except json.JSONDecodeError:
                continue
            except Exception as e:
                print(f"메트릭 읽기 오류: {e}")
                break
        
        # 연결 끊김 처리
        self.is_running = False
        self.connection_lost.emit()
    
    def _process_metrics(self, data: dict):
        """메트릭 데이터 처리 및 버퍼 저장"""
        if 'event' not in data or data['event'] != 'metrics':
            return
            
        # Extract timestamp
        ts = data.get('ts', time.time())
        if isinstance(ts, int):
            ts = ts / 1000.0  # Convert ms to seconds
        
        # Store in buffers
        self._store_metric('net.rtt_ms', ts, data.get('rtt_ms', 0))
        self._store_metric('net.loss_pct', ts, data.get('loss_pct', 0))
        self._store_metric('net.uplink_kbps', ts, data.get('uplink_kbps', 0))
        self._store_metric('sys.cpu_pct', ts, data.get('cpu_pct', 0))
        self._store_metric('sys.gpu_pct', ts, data.get('gpu_pct', 0))
        self._store_metric('sys.mem_mb', ts, data.get('mem_mb', 0))
        
        # OBS metrics
        obs = data.get('obs', {})
        self._store_metric('obs.dropped_ratio', ts, obs.get('dropped_ratio', 0))
        self._store_metric('obs.enc_lag_ms', ts, obs.get('encoding_lag_ms', 0))
        self._store_metric('obs.render_lag_ms', ts, obs.get('render_lag_ms', 0))
        
        # Update latest snapshot
        self.latest_snapshot = data
        
        # Emit signal for immediate UI update
        self.new_metrics.emit(data)
    
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
