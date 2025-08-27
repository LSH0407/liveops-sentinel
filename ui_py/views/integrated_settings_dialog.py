from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
                               QLineEdit, QPushButton, QGroupBox, QFormLayout,
                               QSpinBox, QDoubleSpinBox, QFileDialog, QTabWidget,
                               QWidget, QCheckBox, QMessageBox, QComboBox, QSlider)
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont, QIcon, QPixmap
from pathlib import Path
from core.obs_client_manager import ObsClientManager

class IntegratedSettingsDialog(QDialog):
    """통합 설정 다이얼로그 - 전문가급 디자인"""
    
    def __init__(self, config: dict, parent=None):
        super().__init__(parent)
        self.config = config
        self.obs_manager = ObsClientManager()
        
        self.setWindowTitle("⚙️ LiveOps Sentinel 설정")
        self.setModal(True)
        self.resize(700, 600)
        self.setMinimumSize(600, 500)
        
        self._setup_ui()
        self._setup_connections()
        self._apply_modern_theme()
    
    def _setup_ui(self):
        """UI 초기화 - 전문가급 디자인"""
        layout = QVBoxLayout(self)
        layout.setSpacing(20)
        layout.setContentsMargins(20, 20, 20, 20)
        
        # 헤더
        header_layout = QHBoxLayout()
        header_icon = QLabel("⚙️")
        header_icon.setFont(QFont("Segoe UI", 24))
        header_icon.setStyleSheet("color: #0078d4;")
        
        header_title = QLabel("LiveOps Sentinel 설정")
        header_title.setFont(QFont("Segoe UI", 16, QFont.Bold))
        header_title.setStyleSheet("color: #ffffff;")
        
        header_layout.addWidget(header_icon)
        header_layout.addWidget(header_title)
        header_layout.addStretch()
        
        layout.addLayout(header_layout)
        
        # 탭 위젯 생성
        tab_widget = QTabWidget()
        tab_widget.setFont(QFont("Segoe UI", 10))
        
        # 백엔드 설정 탭
        backend_tab = self._create_backend_tab()
        tab_widget.addTab(backend_tab, "🔧 백엔드")
        
        # 알림 설정 탭
        notification_tab = self._create_notification_tab()
        tab_widget.addTab(notification_tab, "🔔 알림")
        
        # OBS 설정 탭
        obs_tab = self._create_obs_tab()
        tab_widget.addTab(obs_tab, "📹 OBS 연동")
        
        # 진단 설정 탭
        diagnostic_tab = self._create_diagnostic_tab()
        tab_widget.addTab(diagnostic_tab, "🔍 진단")
        
        layout.addWidget(tab_widget)
        
        # 버튼
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        self.cancel_button = QPushButton("❌ 취소")
        self.cancel_button.setMinimumSize(100, 40)
        self.cancel_button.clicked.connect(self.reject)
        
        self.save_button = QPushButton("💾 저장")
        self.save_button.setMinimumSize(120, 40)
        self.save_button.clicked.connect(self.accept)
        self.save_button.setDefault(True)
        
        button_layout.addWidget(self.cancel_button)
        button_layout.addWidget(self.save_button)
        
        layout.addLayout(button_layout)
    
    def _create_backend_tab(self):
        """백엔드 설정 탭 - 전문가급 디자인"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # 백엔드 실행 파일
        backend_group = QGroupBox("🔧 백엔드 실행 파일")
        backend_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        backend_layout = QFormLayout(backend_group)
        backend_layout.setSpacing(10)
        
        self.backend_path_edit = QLineEdit(self.config.get("backend_path", ""))
        self.backend_path_edit.setPlaceholderText("백엔드 실행 파일 경로를 입력하거나 찾기 버튼을 클릭하세요")
        self.backend_path_edit.setMinimumHeight(35)
        
        browse_button = QPushButton("📁 찾기...")
        browse_button.setMinimumHeight(35)
        browse_button.clicked.connect(self._browse_backend)
        
        backend_path_layout = QHBoxLayout()
        backend_path_layout.addWidget(self.backend_path_edit)
        backend_path_layout.addWidget(browse_button)
        
        backend_layout.addRow("백엔드 경로:", backend_path_layout)
        layout.addWidget(backend_group)
        
        # 임계값 설정
        threshold_group = QGroupBox("⚠️ 경고 임계값")
        threshold_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        threshold_layout = QFormLayout(threshold_group)
        threshold_layout.setSpacing(10)
        
        self.response_time = QSpinBox()
        self.response_time.setRange(1, 2000)
        self.response_time.setValue(self.config.get("thresholds", {}).get("rttMs", 100))
        self.response_time.setSuffix(" ms")
        self.response_time.setMinimumHeight(35)
        threshold_layout.addRow("응답시간:", self.response_time)
        
        self.packet_loss = QDoubleSpinBox()
        self.packet_loss.setRange(0, 100)
        self.packet_loss.setDecimals(2)
        self.packet_loss.setValue(self.config.get("thresholds", {}).get("lossPct", 2.0))
        self.packet_loss.setSuffix(" %")
        self.packet_loss.setMinimumHeight(35)
        threshold_layout.addRow("손실률:", self.packet_loss)
        
        self.alert_delay = QSpinBox()
        self.alert_delay.setRange(1, 120)
        self.alert_delay.setValue(self.config.get("thresholds", {}).get("holdSec", 10))
        self.alert_delay.setSuffix(" 초")
        self.alert_delay.setMinimumHeight(35)
        threshold_layout.addRow("지연시간:", self.alert_delay)
        
        layout.addWidget(threshold_group)
        layout.addStretch()
        
        return widget
    
    def _create_notification_tab(self):
        """알림 설정 탭 - 전문가급 디자인"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # Discord 웹후크
        discord_group = QGroupBox("🔔 Discord 알림 설정")
        discord_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        discord_layout = QFormLayout(discord_group)
        discord_layout.setSpacing(10)
        
        self.discord_webhook_edit = QLineEdit(self.config.get("webhook", ""))
        self.discord_webhook_edit.setPlaceholderText("https://discord.com/api/webhooks/... (선택사항)")
        self.discord_webhook_edit.setMinimumHeight(35)
        discord_layout.addRow("Discord Webhook URL:", self.discord_webhook_edit)
        
        # 설명 추가
        discord_info = QLabel("💡 Discord 알림은 선택사항입니다. 비워두고 진행해도 됩니다.")
        discord_info.setStyleSheet("color: #cccccc; font-size: 11px; padding: 10px; background-color: #2a2a2a; border-radius: 5px;")
        discord_layout.addRow("", discord_info)
        
        layout.addWidget(discord_group)
        layout.addStretch()
        
        return widget
    
    def _create_obs_tab(self):
        """OBS 설정 탭 - 전문가급 디자인"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # OBS WebSocket 설정
        obs_group = QGroupBox("📹 OBS WebSocket 연결 설정")
        obs_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        obs_layout = QFormLayout(obs_group)
        obs_layout.setSpacing(10)
        
        # 서버 IP
        obs_config = self.config.get("obs", {})
        self.obs_host_edit = QLineEdit(obs_config.get("host", "127.0.0.1"))
        self.obs_host_edit.setPlaceholderText("127.0.0.1")
        self.obs_host_edit.setMinimumHeight(35)
        obs_layout.addRow("서버 IP:", self.obs_host_edit)
        
        # 서버 포트
        self.obs_port_edit = QLineEdit(str(obs_config.get("port", 4455)))
        self.obs_port_edit.setPlaceholderText("4455")
        self.obs_port_edit.setMinimumHeight(35)
        obs_layout.addRow("서버 포트:", self.obs_port_edit)
        
        # 비밀번호
        self.obs_password_edit = QLineEdit(obs_config.get("password", ""))
        self.obs_password_edit.setPlaceholderText("비밀번호가 설정된 경우에만 입력")
        self.obs_password_edit.setEchoMode(QLineEdit.Password)
        self.obs_password_edit.setMinimumHeight(35)
        obs_layout.addRow("비밀번호:", self.obs_password_edit)
        
        # TLS 체크박스
        self.obs_tls_checkbox = QCheckBox("TLS 사용")
        self.obs_tls_checkbox.setChecked(obs_config.get("use_tls", False))
        obs_layout.addRow("TLS 사용:", self.obs_tls_checkbox)
        
        # OBS 연결 테스트 버튼
        self.obs_test_button = QPushButton("🔗 OBS 연결 테스트")
        self.obs_test_button.setMinimumHeight(40)
        self.obs_manager.set_test_button(self.obs_test_button)
        obs_layout.addRow("", self.obs_test_button)
        
        layout.addWidget(obs_group)
        
        # 설명 텍스트
        info_label = QLabel("""
        💡 OBS Studio에서 WebSocket Server를 활성화해야 합니다:
        1. OBS Studio 실행
        2. 도구 → WebSocket Server Settings
        3. Enable WebSocket server 체크
        4. 위의 설정과 일치하도록 포트/비밀번호/TLS 설정
        """)
        info_label.setWordWrap(True)
        info_label.setStyleSheet("color: #cccccc; font-size: 11px; padding: 15px; background-color: #2a2a2a; border-radius: 5px; line-height: 1.4;")
        layout.addWidget(info_label)
        
        return widget
    
    def _create_diagnostic_tab(self):
        """진단 설정 탭 - 전문가급 디자인"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # 진단 모드 설정
        diagnostic_group = QGroupBox("🔍 진단 모드 설정")
        diagnostic_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        diagnostic_layout = QFormLayout(diagnostic_group)
        diagnostic_layout.setSpacing(10)
        
        # 진단 시간 설정
        self.diagnostic_duration = QComboBox()
        self.diagnostic_duration.addItems([
            "30초",
            "60초",
            "3분",
            "10분",
            "30분",
            "60분",
            "120분",
            "180분"
        ])
        
        # 현재 설정값에 따라 선택
        current_duration = self.config.get("diagnostic_duration_minutes", 60)
        if current_duration <= 30:
            self.diagnostic_duration.setCurrentIndex(0)
        elif current_duration <= 60:
            self.diagnostic_duration.setCurrentIndex(1)
        elif current_duration <= 120:
            self.diagnostic_duration.setCurrentIndex(2)
        else:
            self.diagnostic_duration.setCurrentIndex(3)
        
        self.diagnostic_duration.setMinimumHeight(35)
        diagnostic_layout.addRow("진단 시간:", self.diagnostic_duration)
        
        # 설명
        duration_info = QLabel("💡 진단 모드는 지정된 시간 동안 시스템을 분석하여 최적 설정을 제안합니다.")
        duration_info.setStyleSheet("color: #cccccc; font-size: 11px; padding: 10px; background-color: #2a2a2a; border-radius: 5px;")
        diagnostic_layout.addRow("", duration_info)
        
        layout.addWidget(diagnostic_group)
        layout.addStretch()
        
        return widget
    
    def _browse_backend(self):
        """백엔드 파일 찾기"""
        path, _ = QFileDialog.getOpenFileName(
            self, 
            "백엔드 실행파일 선택", 
            str(Path(self.backend_path_edit.text()).parent) if self.backend_path_edit.text() else "",
            "Executable (*.exe)"
        )
        if path:
            self.backend_path_edit.setText(path)
    
    def _apply_modern_theme(self):
        """현대적인 다크 테마 적용"""
        self.setStyleSheet("""
            QDialog {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                    stop:0 #1a1a1a, stop:1 #2d2d2d);
                color: #ffffff;
                font-family: 'Segoe UI', Arial, sans-serif;
            }
            
            QTabWidget::pane {
                border: 2px solid #404040;
                border-radius: 8px;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #2a2a2a, stop:1 #1e1e1e);
                margin-top: -2px;
            }
            
            QTabBar::tab {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #404040, stop:1 #303030);
                color: #cccccc;
                padding: 12px 20px;
                border: 2px solid #505050;
                border-bottom: none;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                margin-right: 2px;
                font-weight: bold;
                font-size: 11px;
            }
            
            QTabBar::tab:selected {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #0078d4, stop:1 #106ebe);
                color: #ffffff;
                border-color: #0078d4;
            }
            
            QTabBar::tab:hover:!selected {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #505050, stop:1 #404040);
            }
            
            QGroupBox {
                font-weight: bold;
                border: 2px solid #404040;
                border-radius: 10px;
                margin-top: 15px;
                padding-top: 15px;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #2a2a2a, stop:1 #1e1e1e);
            }
            
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 10px 0 10px;
                color: #0078d4;
                font-size: 12px;
            }
            
            QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #404040, stop:1 #303030);
                border: 2px solid #505050;
                border-radius: 6px;
                padding: 8px 12px;
                color: #ffffff;
                font-size: 11px;
                selection-background-color: #0078d4;
            }
            
            QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
                border: 2px solid #0078d4;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #505050, stop:1 #404040);
            }
            
            QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover {
                border: 2px solid #606060;
            }
            
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #404040, stop:1 #303030);
                border: 2px solid #505050;
                border-radius: 6px;
                padding: 10px 20px;
                color: #ffffff;
                font-weight: bold;
                font-size: 11px;
            }
            
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #505050, stop:1 #404040);
                border: 2px solid #606060;
            }
            
            QPushButton:pressed {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #303030, stop:1 #202020);
                border: 2px solid #404040;
            }
            
            QPushButton:default {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #0078d4, stop:1 #106ebe);
                border: 2px solid #0078d4;
                color: #ffffff;
            }
            
            QPushButton:default:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #106ebe, stop:1 #005a9e);
                border: 2px solid #106ebe;
            }
            
            QCheckBox {
                color: #ffffff;
                font-size: 11px;
                spacing: 8px;
            }
            
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border: 2px solid #505050;
                border-radius: 4px;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #404040, stop:1 #303030);
            }
            
            QCheckBox::indicator:checked {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #0078d4, stop:1 #106ebe);
                border: 2px solid #0078d4;
                image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iMTIiIHZpZXdCb3g9IjAgMCAxMiAxMiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTEwIDNMNC41IDguNUwyIDYiIHN0cm9rZT0id2hpdGUiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIi8+Cjwvc3ZnPgo=);
            }
            
            QCheckBox::indicator:hover {
                border: 2px solid #606060;
            }
            
            QLabel {
                color: #ffffff;
                font-size: 11px;
            }
        """)
    
    def _setup_connections(self):
        """시그널 연결"""
        # 필요한 시그널 연결을 여기에 추가
        pass
    
    def get_settings(self) -> dict:
        """설정값 반환"""
        # 진단 시간 설정값 변환
        duration_index = self.diagnostic_duration.currentIndex()
        duration_minutes = [0.5, 1, 3, 10, 30, 60, 120, 180][duration_index]
        
        return {
            'backend_path': self.backend_path_edit.text().strip(),
            'webhook': self.discord_webhook_edit.text().strip(),
            'diagnostic_duration_minutes': duration_minutes,
            'obs': {
                'host': self.obs_host_edit.text().strip() or "127.0.0.1",
                'port': int(self.obs_port_edit.text().strip() or "4455"),
                'use_tls': self.obs_tls_checkbox.isChecked(),
                'password': self.obs_password_edit.text().strip()
            },
            'thresholds': {
                'rttMs': self.response_time.value(),
                'lossPct': self.packet_loss.value(),
                'holdSec': self.alert_delay.value()
            }
        }
