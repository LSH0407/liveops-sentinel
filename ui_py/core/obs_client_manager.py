"""
OBS 클라이언트 관리자
"""

import time
from typing import Optional
from PySide6.QtCore import QObject, QTimer
from PySide6.QtWidgets import QPushButton
from .obs_poller import ObsPoller

class ObsClientManager(QObject):
    """OBS 클라이언트 관리자 - 단일 인스턴스 유지"""
    
    def __init__(self):
        super().__init__()
        self._poller: Optional[ObsPoller] = None
        self._test_button: Optional[QPushButton] = None
        self._cooldown_timer = QTimer()
        self._cooldown_timer.setSingleShot(True)
        self._cooldown_timer.timeout.connect(self._enable_test_button)
    
    def set_test_button(self, button: QPushButton):
        """테스트 버튼 설정"""
        self._test_button = button
        self._test_button.setMinimumHeight(36)  # 최소 높이 36px
        self._test_button.clicked.connect(self._on_test_button_clicked)
    
    def _on_test_button_clicked(self):
        """테스트 버튼 클릭 처리"""
        if self._cooldown_timer.isActive():
            return  # 쿨다운 중
        
        # 버튼 비활성화 및 쿨다운 시작
        self._test_button.setEnabled(False)
        self._test_button.setText("테스트 중...")
        self._cooldown_timer.start(1200)  # 1.2초 쿨다운
        
        # 기존 세션 종료
        self._close_session()
        
        # 새 세션 시도
        self._test_connection()
    
    def _enable_test_button(self):
        """테스트 버튼 활성화"""
        if self._test_button:
            self._test_button.setEnabled(True)
            self._test_button.setText("OBS 연결 테스트")
    
    def _close_session(self):
        """기존 세션 종료"""
        if self._poller:
            try:
                self._poller.stop_polling()
            except:
                pass
            finally:
                self._poller = None
    
    def _test_connection(self):
        """연결 테스트"""
        try:
            # 임시 폴러로 연결 테스트
            temp_poller = ObsPoller()
            success = temp_poller.connect()
            
            if success:
                self._test_button.setText("연결 성공!")
                # 성공 시 폴러 인스턴스 유지
                self._poller = temp_poller
            else:
                self._test_button.setText("연결 실패")
                temp_poller.disconnect()
                
        except Exception as e:
            self._test_button.setText(f"오류: {str(e)[:20]}")
    
    def get_client(self, host: str, port: int, password: str = "", use_tls: bool = False) -> ObsPoller:
        """임시 클라이언트 반환 (테스트용)"""
        return ObsPoller(host, port, password, use_tls)
    
    def get_poller(self) -> Optional[ObsPoller]:
        """현재 폴러 반환"""
        return self._poller
    
    def create_poller(self, host: str, port: int, password: str = "", use_tls: bool = False) -> ObsPoller:
        """새 폴러 생성"""
        # 기존 세션 종료
        self._close_session()
        
        # 새 폴러 생성
        self._poller = ObsPoller(host, port, password, use_tls)
        return self._poller
    
    def start_polling(self):
        """폴링 시작"""
        if self._poller:
            self._poller.start_polling()
    
    def stop_polling(self):
        """폴링 중지"""
        if self._poller:
            self._poller.stop_polling()
    
    def cleanup(self):
        """정리"""
        self._close_session()
        if self._cooldown_timer.isActive():
            self._cooldown_timer.stop()
    
    def get_obs_settings(self) -> dict:
        """OBS 설정 조회"""
        if self._poller:
            return self._poller.get_obs_settings()
        return {
            'output_resolution': 'Unknown',
            'fps': 'Unknown',
            'bitrate': 'Unknown',
            'encoder': 'Unknown'
        }
