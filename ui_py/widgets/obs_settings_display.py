"""
OBS 설정 표시 위젯
"""

from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QFrame
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont

class ObsSettingsDisplay(QWidget):
    """OBS 설정 표시 위젯"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._setup_ui()
        self._update_settings({
            'output_resolution': 'Unknown',
            'fps': 'Unknown',
            'bitrate': 'Unknown',
            'encoder': 'Unknown'
        })
        
        # 셀 크기에 맞춰 확장되도록 설정 (글씨는 충분히 크게 유지)
        from PySide6.QtWidgets import QSizePolicy
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumSize(280, 200)
    
    def _setup_ui(self):
        """UI 설정"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)  # 패딩 대폭 줄임
        layout.setSpacing(2)  # 간격 대폭 줄임
        
        # 제목
        title_label = QLabel("현재 OBS 설정")
        title_font = QFont()
        title_font.setPointSize(10)  # 폰트 크기 더 줄임
        title_font.setBold(True)
        title_label.setFont(title_font)
        title_label.setStyleSheet("color: #E0E0E0; margin-bottom: 2px;")  # 마진 더 줄임
        layout.addWidget(title_label)
        
        # 설정 프레임
        self.settings_frame = QFrame()
        self.settings_frame.setFrameStyle(QFrame.StyledPanel)
        self.settings_frame.setStyleSheet("""
            QFrame {
                background-color: #2A2A2A;
                border: 1px solid #404040;
                border-radius: 4px;
                padding: 4px;
            }
        """)
        
        settings_layout = QVBoxLayout(self.settings_frame)
        settings_layout.setSpacing(2)
        
        # 설정 항목들
        self.resolution_label = self._create_setting_item("해상도", "Unknown")
        self.fps_label = self._create_setting_item("FPS", "Unknown")
        self.bitrate_label = self._create_setting_item("비트레이트", "Unknown")
        self.encoder_label = self._create_setting_item("인코더", "Unknown")
        
        settings_layout.addWidget(self.resolution_label)
        settings_layout.addWidget(self.fps_label)
        settings_layout.addWidget(self.bitrate_label)
        settings_layout.addWidget(self.encoder_label)
        
        layout.addWidget(self.settings_frame)
        layout.addStretch()
    
    def _create_setting_item(self, label_text: str, value_text: str) -> QWidget:
        """설정 항목 위젯 생성"""
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)
        
        # 라벨
        label = QLabel(f"{label_text}:")
        label.setStyleSheet("color: #B0B0B0; font-weight: bold; min-width: 80px;")
        layout.addWidget(label)
        
        # 값
        value = QLabel(value_text)
        value.setStyleSheet("color: #E0E0E0;")
        value.setWordWrap(True)
        layout.addWidget(value)
        
        layout.addStretch()
        return widget
    
    def update_settings(self, settings: dict):
        """설정 업데이트"""
        self._update_settings(settings)
    
    def _update_settings(self, settings: dict):
        """내부 설정 업데이트"""
        # 해상도
        resolution = settings.get('output_resolution', 'Unknown')
        self.resolution_label.findChild(QLabel, "").setText(resolution)
        
        # FPS
        fps = settings.get('fps', 'Unknown')
        self.fps_label.findChild(QLabel, "").setText(fps)
        
        # 비트레이트
        bitrate = settings.get('bitrate', 'Unknown')
        self.bitrate_label.findChild(QLabel, "").setText(bitrate)
        
        # 인코더
        encoder = settings.get('encoder', 'Unknown')
        self.encoder_label.findChild(QLabel, "").setText(encoder)
