"""
OBS Studio WebSocket 클라이언트 (PyInstaller 호환 버전)
"""

import asyncio
import json
import time
import threading
from typing import Dict, Optional, Callable
import websockets
import logging

class ObsClient:
    """OBS Studio WebSocket 클라이언트 (PyInstaller 호환)"""
    
    def __init__(self, parent=None):
        self.websocket = None
        self.connected_state = False
        self.streaming_state = False
        self.last_metrics = {}
        self.connection_thread = None
        self.running = False
        
        # OBS 메트릭 (시뮬레이션)
        self.dropped_frames = 0
        self.encoding_lag_ms = 0
        self.render_lag_ms = 0
        self.fps = 30
        self.bitrate = 6000
        
        # 설정
        self.host = "localhost"
        self.port = 4444
        self.password = ""
        
        # 콜백
        self.on_metrics_callback: Optional[Callable] = None
        self.on_connected_callback: Optional[Callable] = None
        self.on_disconnected_callback: Optional[Callable] = None
        
        # 시뮬레이션 타이머
        self.simulation_timer = None
        self.simulation_thread = None
    
    def set_connection_info(self, host: str = "localhost", port: int = 4444, password: str = ""):
        """연결 정보 설정"""
        self.host = host
        self.port = port
        self.password = password
    
    def start_connection(self):
        """연결 시작 (시뮬레이션)"""
        if self.running:
            return
            
        self.running = True
        self.connected_state = True
        
        # 연결 성공 콜백 호출
        if self.on_connected_callback:
            self.on_connected_callback()
        
        # 시뮬레이션 시작
        self._start_simulation()
    
    def stop_connection(self):
        """연결 중단"""
        self.running = False
        self.connected_state = False
        
        # 시뮬레이션 중단
        self._stop_simulation()
        
        if self.on_disconnected_callback:
            self.on_disconnected_callback()
    
    def is_connected(self) -> bool:
        """연결 상태 확인"""
        return self.connected_state
    
    def is_streaming(self) -> bool:
        """스트리밍 상태 확인"""
        return self.streaming_state
    
    def get_metrics(self) -> Dict:
        """현재 OBS 메트릭 반환"""
        return {
            "dropped_ratio": self.dropped_frames / max(self.fps, 1) if self.fps > 0 else 0,
            "encoding_lag_ms": self.encoding_lag_ms,
            "render_lag_ms": self.render_lag_ms,
            "fps": self.fps,
            "bitrate_kbps": self.bitrate
        }
    
    def set_metrics_callback(self, callback: Callable[[Dict], None]):
        """메트릭 업데이트 콜백 설정"""
        self.on_metrics_callback = callback
    
    def set_connected_callback(self, callback: Callable):
        """연결됨 콜백 설정"""
        self.on_connected_callback = callback
    
    def set_disconnected_callback(self, callback: Callable):
        """연결 해제됨 콜백 설정"""
        self.on_disconnected_callback = callback
    
    def _start_simulation(self):
        """OBS 메트릭 시뮬레이션 시작"""
        if self.simulation_thread and self.simulation_thread.is_alive():
            return
            
        self.simulation_thread = threading.Thread(target=self._simulation_loop, daemon=True)
        self.simulation_thread.start()
    
    def _stop_simulation(self):
        """시뮬레이션 중단"""
        self.running = False
        if self.simulation_thread:
            self.simulation_thread.join(timeout=1)
    
    def _simulation_loop(self):
        """시뮬레이션 루프"""
        import random
        
        while self.running:
            try:
                # 시뮬레이션된 OBS 메트릭 생성
                self.dropped_frames = random.randint(0, 5)
                self.encoding_lag_ms = random.uniform(0, 10)
                self.render_lag_ms = random.uniform(0, 15)
                self.fps = random.uniform(28, 32)
                self.bitrate = random.uniform(5800, 6200)
                
                # 메트릭 업데이트
                metrics = self.get_metrics()
                if self.on_metrics_callback:
                    self.on_metrics_callback(metrics)
                
                # 1초 대기
                time.sleep(1)
                
            except Exception as e:
                print(f"OBS 시뮬레이션 오류: {e}")
                break
