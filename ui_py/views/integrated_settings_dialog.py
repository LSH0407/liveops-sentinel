from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
                               QLineEdit, QPushButton, QGroupBox, QFormLayout,
                               QSpinBox, QDoubleSpinBox, QFileDialog, QTabWidget,
                               QWidget, QCheckBox, QMessageBox)
from PySide6.QtCore import Qt
from pathlib import Path
from core.obs_client import ObsClient

class IntegratedSettingsDialog(QDialog):
    """통합 설정 다이얼로그"""
    
    def __init__(self, config: dict, parent=None):
        super().__init__(parent)
        self.config = config
        self.setWindowTitle("LiveOps Sentinel 설정")
        self.setModal(True)
        self.setFixedSize(500, 400)
        
        self._setup_ui()
        self._apply_dark_theme()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        
        # 탭 위젯 생성
        tab_widget = QTabWidget()
        
        # 백엔드 설정 탭
        backend_tab = self._create_backend_tab()
        tab_widget.addTab(backend_tab, "백엔드")
        
        # 알림 설정 탭
        notification_tab = self._create_notification_tab()
        tab_widget.addTab(notification_tab, "알림")
        
        # OBS 설정 탭
        obs_tab = self._create_obs_tab()
        tab_widget.addTab(obs_tab, "OBS 연동")
        
        layout.addWidget(tab_widget)
        
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
    
    def _create_backend_tab(self):
        """백엔드 설정 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # 백엔드 실행 파일
        backend_group = QGroupBox("백엔드 실행 파일")
        backend_layout = QFormLayout(backend_group)
        
        self.backend_path_edit = QLineEdit(self.config.get("backend_path", ""))
        self.backend_path_edit.setPlaceholderText("백엔드 실행 파일 경로")
        
        browse_button = QPushButton("찾기...")
        browse_button.clicked.connect(self._browse_backend)
        
        backend_path_layout = QHBoxLayout()
        backend_path_layout.addWidget(self.backend_path_edit)
        backend_path_layout.addWidget(browse_button)
        
        backend_layout.addRow("백엔드 경로:", backend_path_layout)
        layout.addWidget(backend_group)
        
        # 임계값 설정
        threshold_group = QGroupBox("경고 임계값")
        threshold_layout = QFormLayout(threshold_group)
        
        self.response_time = QSpinBox()
        self.response_time.setRange(1, 2000)
        self.response_time.setValue(self.config.get("thresholds", {}).get("rttMs", 100))
        self.response_time.setSuffix(" ms")
        threshold_layout.addRow("응답시간:", self.response_time)
        
        self.packet_loss = QDoubleSpinBox()
        self.packet_loss.setRange(0, 100)
        self.packet_loss.setDecimals(2)
        self.packet_loss.setValue(self.config.get("thresholds", {}).get("lossPct", 2.0))
        self.packet_loss.setSuffix(" %")
        threshold_layout.addRow("손실률:", self.packet_loss)
        
        self.alert_delay = QSpinBox()
        self.alert_delay.setRange(1, 120)
        self.alert_delay.setValue(self.config.get("thresholds", {}).get("holdSec", 10))
        self.alert_delay.setSuffix(" 초")
        threshold_layout.addRow("지연시간:", self.alert_delay)
        
        layout.addWidget(threshold_group)
        
        return widget
    
    def _create_notification_tab(self):
        """알림 설정 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Discord 웹후크
        discord_group = QGroupBox("Discord 알림 설정")
        discord_layout = QFormLayout(discord_group)
        
        self.discord_webhook_edit = QLineEdit(self.config.get("webhook", ""))
        self.discord_webhook_edit.setPlaceholderText("https://discord.com/api/webhooks/... (선택사항)")
        discord_layout.addRow("Discord Webhook URL:", self.discord_webhook_edit)
        
        # 설명 추가
        discord_info = QLabel("💡 Discord 알림은 선택사항입니다. 비워두고 진행해도 됩니다.")
        discord_info.setStyleSheet("color: #cccccc; font-size: 11px;")
        discord_layout.addRow("", discord_info)
        
        layout.addWidget(discord_group)
        layout.addStretch()
        
        return widget
    
    def _create_obs_tab(self):
        """OBS 설정 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # OBS WebSocket 설정
        obs_group = QGroupBox("OBS WebSocket 연결 설정")
        obs_layout = QFormLayout(obs_group)
        
        # 서버 IP
        obs_config = self.config.get("obs", {})
        self.obs_host_edit = QLineEdit(obs_config.get("host", "127.0.0.1"))
        self.obs_host_edit.setPlaceholderText("127.0.0.1")
        self.obs_host_edit.setMinimumHeight(30)
        obs_layout.addRow("서버 IP:", self.obs_host_edit)
        
        # 서버 포트
        self.obs_port_edit = QLineEdit(str(obs_config.get("port", 4455)))
        self.obs_port_edit.setPlaceholderText("4455")
        self.obs_port_edit.setMinimumHeight(30)
        obs_layout.addRow("서버 포트:", self.obs_port_edit)
        
        # 비밀번호
        self.obs_password_edit = QLineEdit(obs_config.get("password", ""))
        self.obs_password_edit.setPlaceholderText("비밀번호가 설정된 경우에만 입력")
        self.obs_password_edit.setEchoMode(QLineEdit.Password)
        self.obs_password_edit.setMinimumHeight(30)
        obs_layout.addRow("비밀번호:", self.obs_password_edit)
        
        # TLS 체크박스
        self.obs_tls_checkbox = QCheckBox("TLS 사용")
        self.obs_tls_checkbox.setChecked(obs_config.get("use_tls", False))
        obs_layout.addRow("TLS 사용:", self.obs_tls_checkbox)
        
        # 연결 테스트 버튼
        self.obs_test_button = QPushButton("OBS 연결 테스트")
        self.obs_test_button.clicked.connect(self._test_obs_connection)
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
        info_label.setStyleSheet("color: #cccccc; font-size: 11px;")
        layout.addWidget(info_label)
        
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
    
    def _apply_dark_theme(self):
        """다크 테마 적용"""
        self.setStyleSheet("""
            QDialog {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QTabWidget::pane {
                border: 1px solid #555555;
                background-color: #2b2b2b;
            }
            QTabBar::tab {
                background-color: #404040;
                color: #ffffff;
                padding: 8px 16px;
                border: 1px solid #606060;
                border-bottom: none;
            }
            QTabBar::tab:selected {
                background-color: #505050;
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
            QSpinBox, QDoubleSpinBox {
                background-color: #404040;
                border: 1px solid #606060;
                border-radius: 3px;
                padding: 5px;
                color: #ffffff;
            }
        """)
    
    def get_settings(self):
        """설정값 반환"""
        return {
            'backend_path': self.backend_path_edit.text().strip(),
            'webhook': self.discord_webhook_edit.text().strip(),
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

    def _test_obs_connection(self):
        """OBS 연결 테스트"""
        host = self.obs_host_edit.text().strip() or "127.0.0.1"
        port = int(self.obs_port_edit.text().strip() or "4455")
        password = self.obs_password_edit.text().strip()
        use_tls = self.obs_tls_checkbox.isChecked()

        try:
            obs_client = ObsClient(host, port, password, use_tls)
            success, message = obs_client.test_connect()
            
            if success:
                QMessageBox.information(self, "OBS 연결 테스트", f"✅ {message}")
            else:
                QMessageBox.critical(self, "OBS 연결 실패", f"❌ {message}")
        except Exception as e:
            QMessageBox.critical(self, "OBS 연결 오류", f"연결 테스트 중 오류 발생: {e}")
