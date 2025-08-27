from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
                               QPushButton, QTextEdit, QScrollArea, QWidget,
                               QGroupBox, QTabWidget)
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont

class HelpDialog(QDialog):
    """LiveOps Sentinel 도움말 다이얼로그"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("❓ LiveOps Sentinel 도움말")
        self.setModal(True)
        self.resize(800, 700)
        self.setMinimumSize(700, 600)
        
        self._setup_ui()
        self._apply_theme()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        layout.setSpacing(20)
        layout.setContentsMargins(20, 20, 20, 20)
        
        # 헤더
        header_layout = QHBoxLayout()
        header_icon = QLabel("❓")
        header_icon.setFont(QFont("Segoe UI", 24))
        header_icon.setStyleSheet("color: #0078d4;")
        
        header_title = QLabel("LiveOps Sentinel 도움말")
        header_title.setFont(QFont("Segoe UI", 16, QFont.Bold))
        header_title.setStyleSheet("color: #ffffff;")
        
        header_layout.addWidget(header_icon)
        header_layout.addWidget(header_title)
        header_layout.addStretch()
        
        layout.addLayout(header_layout)
        
        # 탭 위젯
        tab_widget = QTabWidget()
        tab_widget.setFont(QFont("Segoe UI", 10))
        
        # 프로그램 소개 탭
        intro_tab = self._create_intro_tab()
        tab_widget.addTab(intro_tab, "📋 프로그램 소개")
        
        # 모니터링 요소 탭
        metrics_tab = self._create_metrics_tab()
        tab_widget.addTab(metrics_tab, "📊 모니터링 요소")
        
        # 측정 방법 탭
        measurement_tab = self._create_measurement_tab()
        tab_widget.addTab(measurement_tab, "🔬 측정 방법")
        
        layout.addWidget(tab_widget)
        
        # 닫기 버튼
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        close_button = QPushButton("❌ 닫기")
        close_button.setMinimumSize(100, 40)
        close_button.clicked.connect(self.accept)
        close_button.setDefault(True)
        
        button_layout.addWidget(close_button)
        layout.addLayout(button_layout)
    
    def _create_intro_tab(self):
        """프로그램 소개 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # 프로그램 설명
        intro_group = QGroupBox("🎯 LiveOps Sentinel이란?")
        intro_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        intro_layout = QVBoxLayout(intro_group)
        
        intro_text = QTextEdit()
        intro_text.setReadOnly(True)
        intro_text.setMaximumHeight(200)
        intro_text.setHtml("""
        <h3>실시간 스트리밍 모니터링 시스템</h3>
        <p><b>LiveOps Sentinel</b>은 실시간 스트리밍 환경에서 네트워크, 시스템, OBS 상태를 종합적으로 모니터링하고 
        문제를 사전에 감지하여 알림을 제공하는 전문적인 모니터링 도구입니다.</p>
        
        <h4>주요 기능:</h4>
        <ul>
        <li><b>실시간 네트워크 모니터링</b>: RTT, 패킷 손실, 대역폭 측정</li>
        <li><b>시스템 리소스 모니터링</b>: CPU, GPU, 메모리 사용률 추적</li>
        <li><b>OBS Studio 통합</b>: 스트리밍/녹화 상태, 드롭된 프레임 모니터링</li>
        <li><b>스마트 알림</b>: Discord, Slack을 통한 실시간 알림</li>
        <li><b>성능 최적화</b>: 자동 메모리 관리 및 CPU 최적화</li>
        <li><b>진단 모드</b>: 시스템 분석 및 최적 설정 제안</li>
        </ul>
        """)
        intro_layout.addWidget(intro_text)
        layout.addWidget(intro_group)
        
        # 사용 목적
        purpose_group = QGroupBox("🎯 사용 목적")
        purpose_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        purpose_layout = QVBoxLayout(purpose_group)
        
        purpose_text = QTextEdit()
        purpose_text.setReadOnly(True)
        purpose_text.setMaximumHeight(150)
        purpose_text.setHtml("""
        <h4>스트리밍 품질 보장:</h4>
        <ul>
        <li>네트워크 문제 사전 감지 및 대응</li>
        <li>시스템 성능 최적화</li>
        <li>OBS 설정 자동 조정</li>
        <li>시청자 경험 향상</li>
        </ul>
        
        <h4>운영 효율성 증대:</h4>
        <ul>
        <li>실시간 모니터링으로 수동 점검 시간 절약</li>
        <li>자동화된 알림 시스템</li>
        <li>데이터 기반 의사결정 지원</li>
        </ul>
        """)
        purpose_layout.addWidget(purpose_text)
        layout.addWidget(purpose_group)
        
        layout.addStretch()
        return widget
    
    def _create_metrics_tab(self):
        """모니터링 요소 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # 네트워크 메트릭
        network_group = QGroupBox("🌐 네트워크 메트릭")
        network_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        network_layout = QVBoxLayout(network_group)
        
        network_text = QTextEdit()
        network_text.setReadOnly(True)
        network_text.setMaximumHeight(200)
        network_text.setHtml("""
        <h4>1. 서버 응답 속도 (RTT)</h4>
        <p><b>정의:</b> 네트워크 패킷이 서버까지 왕복하는데 걸리는 시간</p>
        <p><b>측정:</b> ICMP ping을 통한 실시간 측정</p>
        <p><b>기준:</b> 20ms 이하(좋음), 80ms 이하(주의), 150ms 초과(불안정)</p>
        
        <h4>2. 전송 손실</h4>
        <p><b>정의:</b> 네트워크에서 손실되는 패킷의 비율</p>
        <p><b>측정:</b> UDP 패킷 전송/수신 비율 계산</p>
        <p><b>기준:</b> 0% (좋음), 0.5% 이하(주의), 2% 초과(불안정)</p>
        
        <h4>3. 업로드 여유율</h4>
        <p><b>정의:</b> 현재 업로드 속도 대비 사용 가능한 대역폭</p>
        <p><b>측정:</b> 실제 업로드 속도 측정 후 여유율 계산</p>
        <p><b>기준:</b> 50% 이상(좋음), 20% 이상(주의), 0% 미만(불안정)</p>
        """)
        network_layout.addWidget(network_text)
        layout.addWidget(network_group)
        
        # 시스템 메트릭
        system_group = QGroupBox("💻 시스템 메트릭")
        system_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        system_layout = QVBoxLayout(system_group)
        
        system_text = QTextEdit()
        system_text.setReadOnly(True)
        system_text.setMaximumHeight(200)
        system_text.setHtml("""
        <h4>4. CPU 사용률</h4>
        <p><b>정의:</b> 전체 CPU 코어의 평균 사용률</p>
        <p><b>측정:</b> Windows Performance Counters API 사용</p>
        <p><b>기준:</b> 50% 이하(좋음), 80% 이하(주의), 90% 초과(불안정)</p>
        
        <h4>5. GPU 사용률</h4>
        <p><b>정의:</b> 그래픽 카드의 사용률 (주로 인코딩용)</p>
        <p><b>측정:</b> NVIDIA/AMD GPU API를 통한 실시간 측정</p>
        <p><b>기준:</b> 60% 이하(좋음), 85% 이하(주의), 95% 초과(불안정)</p>
        
        <h4>6. 메모리 사용률</h4>
        <p><b>정의:</b> 시스템 RAM의 사용량</p>
        <p><b>측정:</b> Windows Memory API를 통한 실시간 측정</p>
        <p><b>기준:</b> 70% 이하(좋음), 90% 이하(주의), 95% 초과(불안정)</p>
        """)
        system_layout.addWidget(system_text)
        layout.addWidget(system_group)
        
        # OBS 메트릭
        obs_group = QGroupBox("📹 OBS Studio 메트릭")
        obs_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        obs_layout = QVBoxLayout(obs_group)
        
        obs_text = QTextEdit()
        obs_text.setReadOnly(True)
        obs_text.setMaximumHeight(200)
        obs_text.setHtml("""
        <h4>7. 버린 프레임 비율</h4>
        <p><b>정의:</b> 인코딩 과정에서 손실된 프레임의 비율</p>
        <p><b>측정:</b> OBS WebSocket API를 통한 실시간 측정</p>
        <p><b>기준:</b> 1% 이하(좋음), 3% 이하(주의), 5% 초과(불안정)</p>
        
        <h4>8. 인코딩 지연</h4>
        <p><b>정의:</b> 비디오 인코딩에 걸리는 시간</p>
        <p><b>측정:</b> OBS 인코더 통계 API 사용</p>
        <p><b>기준:</b> 5ms 이하(좋음), 10ms 이하(주의), 20ms 초과(불안정)</p>
        
        <h4>9. 렌더 지연</h4>
        <p><b>정의:</b> 화면 렌더링에 걸리는 시간</p>
        <p><b>측정:</b> OBS 렌더링 통계 API 사용</p>
        <p><b>기준:</b> 7ms 이하(좋음), 14ms 이하(주의), 25ms 초과(불안정)</p>
        """)
        obs_layout.addWidget(obs_text)
        layout.addWidget(obs_group)
        
        layout.addStretch()
        return widget
    
    def _create_measurement_tab(self):
        """측정 방법 탭"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(15)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # 측정 기술
        tech_group = QGroupBox("🔬 측정 기술")
        tech_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        tech_layout = QVBoxLayout(tech_group)
        
        tech_text = QTextEdit()
        tech_text.setReadOnly(True)
        tech_text.setMaximumHeight(200)
        tech_text.setHtml("""
        <h4>네트워크 측정:</h4>
        <ul>
        <li><b>ICMP Ping:</b> 서버 응답 시간 측정 (1초 간격)</li>
        <li><b>UDP 패킷 전송:</b> 패킷 손실률 측정</li>
        <li><b>대역폭 테스트:</b> 실제 업로드 속도 측정</li>
        </ul>
        
        <h4>시스템 측정:</h4>
        <ul>
        <li><b>Windows Performance Counters:</b> CPU, 메모리 사용률</li>
        <li><b>GPU API:</b> NVIDIA CUDA, AMD ROCm API</li>
        <li><b>실시간 폴링:</b> 1초 간격으로 지속적 측정</li>
        </ul>
        
        <h4>OBS 측정:</h4>
        <ul>
        <li><b>WebSocket API:</b> OBS Studio v5 프로토콜</li>
        <li><b>실시간 통계:</b> 인코더/렌더링 통계 수집</li>
        <li><b>자동 재연결:</b> 연결 끊김 시 자동 복구</li>
        </ul>
        """)
        tech_layout.addWidget(tech_text)
        layout.addWidget(tech_group)
        
        # 계산 방법
        calc_group = QGroupBox("🧮 계산 방법")
        calc_group.setFont(QFont("Segoe UI", 11, QFont.Bold))
        calc_layout = QVBoxLayout(calc_group)
        
        calc_text = QTextEdit()
        calc_text.setReadOnly(True)
        calc_text.setMaximumHeight(300)
        calc_text.setHtml("""
        <h4>품질 점수 계산:</h4>
        <p><b>네트워크 점수 (40%):</b></p>
        <ul>
        <li>RTT 점수 (50%): 20ms=100점, 80ms=60점, 150ms=30점</li>
        <li>손실률 점수 (30%): 0%=100점, 0.5%=80점, 2%=40점</li>
        <li>업로드 여유율 점수 (20%): 50%+=100점, 20%+=70점</li>
        </ul>
        
        <p><b>시스템 점수 (30%):</b></p>
        <ul>
        <li>CPU 점수 (60%): 사용률이 낮을수록 높은 점수</li>
        <li>GPU 점수 (40%): 사용률이 낮을수록 높은 점수</li>
        </ul>
        
        <p><b>OBS 점수 (30%):</b></p>
        <ul>
        <li>버린 프레임 점수 (50%): 1% 이하=100점, 3% 이하=70점</li>
        <li>인코딩 지연 점수 (25%): 5ms 이하=100점, 10ms 이하=70점</li>
        <li>렌더 지연 점수 (25%): 7ms 이하=100점, 14ms 이하=70점</li>
        </ul>
        
        <h4>등급 판정:</h4>
        <ul>
        <li><b>좋음 (85점 이상):</b> 현재 설정이 최적입니다</li>
        <li><b>주의 (60-84점):</b> 모니터링 필요, 일부 조정 권장</li>
        <li><b>불안정 (60점 미만):</b> 즉시 조치 필요</li>
        </ul>
        """)
        calc_layout.addWidget(calc_text)
        layout.addWidget(calc_group)
        
        layout.addStretch()
        return widget
    
    def _apply_theme(self):
        """다크 테마 적용"""
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
            
            QTextEdit {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #2a2a2a, stop:1 #1e1e1e);
                border: 2px solid #404040;
                border-radius: 6px;
                color: #ffffff;
                font-size: 11px;
                selection-background-color: #0078d4;
            }
            
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #0078d4, stop:1 #106ebe);
                border: 2px solid #0078d4;
                border-radius: 6px;
                padding: 10px 20px;
                color: #ffffff;
                font-weight: bold;
                font-size: 11px;
            }
            
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                    stop:0 #106ebe, stop:1 #005a9e);
                border: 2px solid #106ebe;
            }
            
            QLabel {
                color: #ffffff;
                font-size: 11px;
            }
        """)
