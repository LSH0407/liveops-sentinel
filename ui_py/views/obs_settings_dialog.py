from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
                               QLineEdit, QPushButton, QGroupBox, QFormLayout)
from PySide6.QtCore import Qt

class ObsSettingsDialog(QDialog):
    """OBS WebSocket 설정 다이얼로그"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("OBS WebSocket 설정")
        self.setModal(True)
        self.setFixedSize(400, 250)
        
        # 기본값
        self.host = "localhost"
        self.port = 4455
        self.password = ""
        
        self._setup_ui()
        self._apply_dark_theme()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        
        # OBS WebSocket 설정 그룹
        obs_group = QGroupBox("OBS WebSocket 연결 설정")
        obs_layout = QFormLayout(obs_group)
        
        # 서버 IP
        self.host_edit = QLineEdit(self.host)
        self.host_edit.setPlaceholderText("localhost")
        obs_layout.addRow("서버 IP:", self.host_edit)
        
        # 서버 포트
        self.port_edit = QLineEdit(str(self.port))
        self.port_edit.setPlaceholderText("4455")
        obs_layout.addRow("서버 포트:", self.port_edit)
        
        # 비밀번호
        self.password_edit = QLineEdit(self.password)
        self.password_edit.setPlaceholderText("비밀번호가 설정된 경우에만 입력")
        self.password_edit.setEchoMode(QLineEdit.Password)
        obs_layout.addRow("비밀번호:", self.password_edit)
        
        layout.addWidget(obs_group)
        
        # 설명 텍스트
        info_label = QLabel("""
        💡 OBS Studio에서 WebSocket Server를 활성화해야 합니다:
        1. OBS Studio 실행
        2. 도구 → WebSocket Server Settings
        3. Enable WebSocket server 체크
        4. 위의 설정과 일치하도록 포트/비밀번호 설정
        """)
        info_label.setWordWrap(True)
        info_label.setStyleSheet("color: #cccccc; font-size: 11px;")
        layout.addWidget(info_label)
        
        # 버튼
        button_layout = QHBoxLayout()
        
        self.cancel_button = QPushButton("취소")
        self.cancel_button.clicked.connect(self.reject)
        
        self.save_button = QPushButton("저장")
        self.save_button.clicked.connect(self.accept)
        self.save_button.setDefault(True)
        
        button_layout.addWidget(self.cancel_button)
        button_layout.addWidget(self.save_button)
        
        layout.addLayout(button_layout)
    
    def _apply_dark_theme(self):
        """다크 테마 적용"""
        self.setStyleSheet("""
            QDialog {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QGroupBox {
                font-weight: bold;
                border: 1px solid #555555;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QLineEdit {
                background-color: #404040;
                border: 1px solid #606060;
                border-radius: 3px;
                padding: 5px;
                color: #ffffff;
            }
            QLineEdit:focus {
                border: 1px solid #0078d4;
            }
            QPushButton {
                background-color: #404040;
                border: 1px solid #606060;
                border-radius: 3px;
                padding: 8px 16px;
                color: #ffffff;
            }
            QPushButton:hover {
                background-color: #505050;
            }
            QPushButton:pressed {
                background-color: #303030;
            }
            QPushButton:default {
                background-color: #0078d4;
                border: 1px solid #0078d4;
            }
            QPushButton:default:hover {
                background-color: #106ebe;
            }
        """)
    
    def get_settings(self):
        """설정값 반환"""
        return {
            'host': self.host_edit.text().strip() or "localhost",
            'port': int(self.port_edit.text().strip() or "4455"),
            'password': self.password_edit.text().strip()
        }
    
    def set_settings(self, host: str, port: int, password: str):
        """설정값 설정"""
        self.host_edit.setText(host)
        self.port_edit.setText(str(port))
        self.password_edit.setText(password)
