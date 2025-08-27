from PySide6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                                QLabel, QPushButton, QFrame, QGridLayout, QSplitter,
                                QGroupBox, QMessageBox, QApplication, QComboBox, QCheckBox, QSizePolicy)
from PySide6.QtCore import Qt, QTimer, QThread, Signal
from PySide6.QtGui import QFont, QIcon
import time
from typing import List, Dict, Optional

from widgets.status_card import StatusCard
from widgets.gauge import GaugeWidget
# from widgets.metric_graph import PyQtGraphWidget, MetricGraph  # 사용하지 않음
from widgets.simple_graph import SimpleGraphWidget
from widgets.obs_settings_display import ObsSettingsDisplay
from core.metric_bus import MetricBus
from core.obs_client_manager import ObsClientManager
from core.score import QualityScore
from core.stream_health_monitor import StreamHealthMonitor
from views.integrated_settings_dialog import IntegratedSettingsDialog
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from settings import load, save
from platform_rules import get_platform_display_names

class DashboardView(QWidget):
    """메인 대시보드 뷰"""
    
    def __init__(self, metric_bus=None, config=None, parent=None):
        super().__init__(parent)
        
        # 설정 로드
        self.config = config or load()
        
        # OBS 클라이언트 매니저 초기화
        self.obs_manager = ObsClientManager()
        
        # OBS 설정
        obs_config = self.config.get("obs", {})
        obs_host = obs_config.get("host", "127.0.0.1")
        obs_port = obs_config.get("port", 4455)
        obs_password = obs_config.get("password", "")
        obs_use_tls = obs_config.get("use_tls", False)
        
        # OBS 폴러 생성
        self.obs_poller = self.obs_manager.create_poller(obs_host, obs_port, obs_password, obs_use_tls)
        self.obs_poller.tick.connect(self._on_obs_metrics_updated)
        self.obs_poller.connected.connect(self._on_obs_connected)
        self.obs_poller.disconnected.connect(self._on_obs_disconnected)
        
        # 메트릭 버스
        self.metric_bus = metric_bus
        if self.metric_bus:
            self.metric_bus.new_metrics.connect(self._on_metrics_update)
        
        # 품질 점수 계산기
        self.quality_score = QualityScore()
        
        # 스트림 헬스 모니터
        self.stream_health_monitor = StreamHealthMonitor()
        self.stream_health_monitor.stream_interruption_detected.connect(self._on_stream_interruption)
        self.stream_health_monitor.stream_quality_degraded.connect(self._on_quality_degradation)
        self.stream_health_monitor.stream_recovered.connect(self._on_stream_recovered)
        
        # UI 상태
        self.monitoring_active = False
        self.simple_mode = False
        
        # 메트릭 윈도우 초기화
        self.metrics_window = []
        
        # UI 초기화
        self._setup_ui()
        self._setup_connections()
        
        # 모니터링 시작
        self.metric_bus.start()
        
        # OBS 폴링 자동 시작
        if hasattr(self, 'obs_poller'):
            print("OBS 폴러 연결 시도 중...")
            if self.obs_poller.connect():
                print("OBS 폴러 연결 성공")
                self.obs_poller.start_polling()
                print("OBS 폴링 자동 시작됨")
                
                # 연결 즉시 설정 조회
                try:
                    obs_settings = self.obs_poller.get_obs_settings()
                    print(f"초기 OBS 설정: {obs_settings}")
                    self.obs_settings_display.update_settings(obs_settings)
                except Exception as e:
                    print(f"초기 OBS 설정 조회 실패: {e}")
            else:
                print("OBS 폴러 연결 실패")
        
        print("대시보드 초기화 완료")
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # Top bar with controls
        top_bar = self._create_top_bar()
        layout.addWidget(top_bar)
        
        # Main content splitter
        splitter = QSplitter(Qt.Horizontal)
        
        # Left panel: KPI cards and gauge
        left_panel = self._create_left_panel()
        splitter.addWidget(left_panel)
        
        # Right panel: Graphs
        right_panel = self._create_right_panel()
        splitter.addWidget(right_panel)
        
        # Set splitter proportions
        splitter.setSizes([400, 800])
        splitter.setStretchFactor(0, 1)  # 왼쪽 패널
        splitter.setStretchFactor(1, 2)  # 오른쪽 패널
        layout.addWidget(splitter)
        
        # Bottom bar: Recommendations
        bottom_bar = self._create_bottom_bar()
        layout.addWidget(bottom_bar)
        
        # 디버그 정보 표시용 라벨 추가
        self.debug_label = QLabel("디버그: 대기 중")
        self.debug_label.setStyleSheet("""
            QLabel {
                color: #ffff00;
                font-size: 10px;
                padding: 2px;
                background-color: #333333;
                border: 1px solid #555555;
            }
        """)
        layout.addWidget(self.debug_label)
    
    def _create_top_bar(self) -> QWidget:
        """상단 컨트롤 바 생성"""
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        
        # 백엔드 연결 상태 표시
        self.backend_status_label = QLabel("백엔드: 연결 대기 중")
        self.backend_status_label.setStyleSheet("""
            QLabel {
                color: #ffaa00;
                font-size: 11px;
                padding: 4px;
            }
        """)
        layout.addWidget(self.backend_status_label)
        
        # Title
        title = QLabel("LiveOps Sentinel • 모니터링")
        title.setStyleSheet("""
            QLabel {
                color: #ffffff;
                font-size: 18px;
                font-weight: bold;
            }
        """)
        layout.addWidget(title)
        
        # Platform selection
        platform_label = QLabel("플랫폼:")
        platform_label.setStyleSheet("color: #e0e0e0; font-size: 12px;")
        layout.addWidget(platform_label)
        
        self.platform_combo = QComboBox()
        self.platform_combo.setStyleSheet("""
            QComboBox {
                background-color: #404040;
                color: #ffffff;
                border: 1px solid #606060;
                border-radius: 4px;
                padding: 4px 8px;
                font-size: 12px;
                min-width: 120px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox::down-arrow {
                image: none;
                border-left: 5px solid transparent;
                border-right: 5px solid transparent;
                border-top: 5px solid #ffffff;
            }
        """)
        
        # 플랫폼 목록 추가
        platform_names = get_platform_display_names()
        for key, name in platform_names.items():
            self.platform_combo.addItem(name, key)
        
        self.platform_combo.currentTextChanged.connect(self._on_platform_changed)
        layout.addWidget(self.platform_combo)
        
        # 설정 버튼 추가
        self.settings_button = QPushButton("설정")
        self.settings_button.setStyleSheet("""
            QPushButton {
                background-color: #404040;
                border: 1px solid #606060;
                border-radius: 4px;
                padding: 4px 8px;
                color: #ffffff;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #505050;
            }
        """)
        self.settings_button.clicked.connect(self._open_settings)
        layout.addWidget(self.settings_button)
        
        layout.addStretch()
        
        # 실시간 모니터링 시작 버튼
        self.monitoring_btn = QPushButton("실시간 모니터링 시작")
        self.monitoring_btn.setStyleSheet("""
            QPushButton {
                background-color: #dc3545;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-size: 12px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #c82333;
            }
        """)
        self.monitoring_btn.clicked.connect(self._toggle_monitoring)
        layout.addWidget(self.monitoring_btn)
        
        # 도움말 버튼
        self.help_btn = QPushButton("❓")
        self.help_btn.setStyleSheet("""
            QPushButton {
                background-color: #2d2d2d;
                color: #ffffff;
                border: 2px solid #404040;
                padding: 8px 12px;
                border-radius: 6px;
                font-size: 14px;
                font-weight: bold;
                min-width: 30px;
            }
            QPushButton:hover {
                background-color: #404040;
                border-color: #505050;
            }
        """)
        self.help_btn.clicked.connect(self._show_help)
        layout.addWidget(self.help_btn)
        
        # Simple/Expert mode toggle
        self.mode_toggle = QCheckBox("간단 모드")
        self.mode_toggle.setStyleSheet("""
            QCheckBox {
                color: #e0e0e0;
                font-size: 12px;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
            }
        """)
        self.mode_toggle.toggled.connect(self._toggle_mode)
        layout.addWidget(self.mode_toggle)
        
        # Diagnostic button
        diagnostic_btn = QPushButton("진단 모드")
        diagnostic_btn.setStyleSheet("""
            QPushButton {
                background-color: #28a745;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #218838;
            }
        """)
        diagnostic_btn.clicked.connect(self._run_diagnostic)
        layout.addWidget(diagnostic_btn)
        
        # Benchmark button
        benchmark_btn = QPushButton("10초 벤치마크")
        benchmark_btn.setStyleSheet("""
            QPushButton {
                background-color: #007bff;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #0056b3;
            }
        """)
        benchmark_btn.clicked.connect(self._run_benchmark)
        layout.addWidget(benchmark_btn)
        
        return widget
    
    def _create_left_panel(self) -> QWidget:
        """왼쪽 패널 (KPI 카드 + 게이지) 생성"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)
        
        # Quality gauge
        self.quality_gauge = GaugeWidget("품질 점수")
        layout.addWidget(self.quality_gauge, alignment=Qt.AlignCenter)
        
        # KPI cards grid
        cards_layout = QGridLayout()
        cards_layout.setSpacing(8)
        
        # Network cards
        self.rtt_card = StatusCard("서버 응답 속도", "ms")
        self.loss_card = StatusCard("전송 손실", "%")
        self.uplink_card = StatusCard("업로드 여유율", "%")
        
        cards_layout.addWidget(self.rtt_card, 0, 0)
        cards_layout.addWidget(self.loss_card, 0, 1)
        cards_layout.addWidget(self.uplink_card, 0, 2)
        
        # System cards
        self.cpu_card = StatusCard("CPU", "%")
        self.gpu_card = StatusCard("GPU", "%")
        self.mem_card = StatusCard("메모리", "MB")
        
        cards_layout.addWidget(self.cpu_card, 1, 0)
        cards_layout.addWidget(self.gpu_card, 1, 1)
        cards_layout.addWidget(self.mem_card, 1, 2)
        
        # OBS cards (expert mode only)
        self.dropped_card = StatusCard("프레임 드랍", "%")
        self.enc_lag_card = StatusCard("인코딩 지연", "ms")
        self.render_lag_card = StatusCard("렌더 지연", "ms")
        
        cards_layout.addWidget(self.dropped_card, 2, 0)
        cards_layout.addWidget(self.enc_lag_card, 2, 1)
        cards_layout.addWidget(self.render_lag_card, 2, 2)
        
        layout.addLayout(cards_layout)
        
        return widget
    
    def _create_right_panel(self) -> QWidget:
        """오른쪽 패널 (그래프) 생성"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)
        
        # 기본설정 그래프 그룹
        basic_group = QGroupBox("시스템 성능 모니터링")
        basic_group.setStyleSheet("""
            QGroupBox {
                color: #e0e0e0;
                font-weight: bold;
                border: 2px solid #444;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        """)
        basic_layout = QGridLayout(basic_group)
        basic_layout.setSpacing(10)
        
        # Network graphs
        self.rtt_graph = SimpleGraphWidget("서버 응답 속도 (RTT)")
        self.loss_graph = SimpleGraphWidget("전송 손실")
        
        # System graphs
        self.cpu_graph = SimpleGraphWidget("CPU 사용률")
        self.gpu_graph = SimpleGraphWidget("GPU 사용률")
        
        # 2x2 그리드로 배치
        basic_layout.addWidget(self.rtt_graph, 0, 0)
        basic_layout.addWidget(self.loss_graph, 0, 1)
        basic_layout.addWidget(self.cpu_graph, 1, 0)
        basic_layout.addWidget(self.gpu_graph, 1, 1)
        
        # 4등분 배열을 위한 설정 (OBS Studio 메트릭과 동일)
        basic_layout.setColumnStretch(0, 1)
        basic_layout.setColumnStretch(1, 1)
        basic_layout.setRowStretch(0, 1)
        basic_layout.setRowStretch(1, 1)
        
        layout.addWidget(basic_group)
        
        # OBS 그래프 (전문 모드)
        obs_group = QGroupBox("OBS Studio 메트릭")
        obs_group.setStyleSheet("""
            QGroupBox {
                color: #e0e0e0;
                font-weight: bold;
                border: 2px solid #444;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        """)
        obs_layout = QGridLayout(obs_group)
        obs_layout.setSpacing(10)
        obs_layout.setContentsMargins(10, 10, 10, 10)
        
        # SimpleGraphWidget 기반 OBS 그래프 (pyqtgraph 대신)
        self.dropped_graph = SimpleGraphWidget("프레임 드랍 비율")
        self.enc_lag_graph = SimpleGraphWidget("인코딩 지연")
        self.render_lag_graph = SimpleGraphWidget("렌더 지연")
        
        # OBS 설정 표시 위젯
        self.obs_settings_display = ObsSettingsDisplay()
        
        # 2x2 그리드로 배치 (기본설정 모니터링과 동일한 크기)
        obs_layout.addWidget(self.dropped_graph, 0, 0)
        obs_layout.addWidget(self.enc_lag_graph, 0, 1)
        obs_layout.addWidget(self.render_lag_graph, 1, 0)
        obs_layout.addWidget(self.obs_settings_display, 1, 1)
        
        # 4등분 배열을 위한 추가 설정
        obs_layout.setColumnStretch(0, 1)
        obs_layout.setColumnStretch(1, 1)
        obs_layout.setRowStretch(0, 1)
        obs_layout.setRowStretch(1, 1)
        
        # 그리드 비율 설정 (모든 셀이 동일한 크기)
        obs_layout.setColumnStretch(0, 1)
        obs_layout.setColumnStretch(1, 1)
        obs_layout.setRowStretch(0, 1)
        obs_layout.setRowStretch(1, 1)
        
        # 각 위젯의 크기 정책 설정
        self.dropped_graph.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.enc_lag_graph.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.render_lag_graph.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.obs_settings_display.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        layout.addWidget(obs_group)
        
        return widget
    
    def _create_bottom_bar(self) -> QWidget:
        """하단 권장사항 바 생성"""
        widget = QFrame()
        widget.setFrameStyle(QFrame.StyledPanel)
        layout = QVBoxLayout(widget)
        
        # Recommendation title
        title = QLabel("권장 조치")
        title.setStyleSheet("""
            QLabel {
                color: #ffffff;
                font-size: 14px;
                font-weight: bold;
            }
        """)
        layout.addWidget(title)
        
        # Recommendation text
        self.recommendation_label = QLabel("모니터링을 시작하세요...")
        self.recommendation_label.setStyleSheet("""
            QLabel {
                color: #b0b0b0;
                font-size: 12px;
                padding: 8px;
                background-color: #1e1e1e;
                border-radius: 4px;
            }
        """)
        self.recommendation_label.setWordWrap(True)
        layout.addWidget(self.recommendation_label)
        
        return widget
    
    def _setup_connections(self):
        """시그널 연결"""
        # Subscribe to metric updates
        self.metric_bus.new_metrics.connect(self._on_metrics_update)
        
        # 백엔드 연결 상태 모니터링
        self.metric_bus.connection_lost.connect(self._on_backend_disconnected)
        self.metric_bus.connection_established.connect(self._on_backend_connected)
        
        # Setup graphs
        self._setup_graphs()
    
    def _setup_graphs(self):
        """그래프 설정"""
        print("=== 그래프 설정 시작 ===")
        
        # PyQtGraph 기반 그래프들 (OBS 그래프들) - 초기에 활성화
        self.dropped_graph.set_active(True)  # 활성화로 변경
        self.enc_lag_graph.set_active(True)  # 활성화로 변경
        self.render_lag_graph.set_active(True)  # 활성화로 변경
        
        # 간단한 테스트 데이터 추가
        current_time = time.time()
        self.dropped_graph.add_point(current_time, 2.5)
        self.enc_lag_graph.add_point(current_time, 5.0)
        self.render_lag_graph.add_point(current_time, 3.0)
        
        print("그래프 설정 완료 - OBS 그래프들 활성화됨")
        print("=== 그래프 설정 끝 ===")
    
    def _apply_dark_theme(self):
        """다크 테마 적용"""
        self.setStyleSheet("""
            DashboardView {
                background-color: #1e1e1e;
                color: #ffffff;
            }
        """)
    
    def _on_metrics_update(self, metrics: dict):
        """메트릭 업데이트 처리"""
        # print(f"대시보드 메트릭 업데이트: {metrics}")
        
        # 모니터링이 활성화된 경우에만 업데이트
        if not self.monitoring_active:
            # print("모니터링이 비활성화되어 있음")
            return
            
        # print("모니터링 활성화됨 - 메트릭 처리 중")
        
        # 디버그 정보 업데이트
        cpu = metrics.get('cpu_pct', 0)
        gpu = metrics.get('gpu_pct', 0)
        rtt = metrics.get('rtt_ms', 0)
        self.debug_label.setText(f"디버그: CPU={cpu:.1f}%, GPU={gpu:.1f}%, RTT={rtt:.1f}ms")
        
        # Store in window for scoring
        self.metrics_window.append(metrics)
        if len(self.metrics_window) > 50:  # Keep last 50 samples
            self.metrics_window.pop(0)
        
        # print(f"메트릭 윈도우에 저장됨: 현재 {len(self.metrics_window)}개 샘플")
        
        # Update KPI cards
        self._update_kpi_cards(metrics)
        
        # Update graphs
        self._update_graphs(metrics)
        
        # Update quality score
        self._update_quality_score()
        
        # 스트림 헬스 모니터 업데이트
        self.stream_health_monitor.update_metrics(metrics)
    
    def _update_kpi_cards(self, metrics: dict):
        """KPI 카드 업데이트"""
        # Network metrics
        rtt = metrics.get('rtt_ms', 0)
        self.rtt_card.set_value(rtt)
        self.rtt_card.set_grade(self._get_grade_for_rtt(rtt))
        
        loss = metrics.get('loss_pct', 0)
        self.loss_card.set_value(loss)
        self.loss_card.set_grade(self._get_grade_for_loss(loss))
        
        # Calculate uplink headroom
        uplink = metrics.get('uplink_kbps', 0)
        if self.current_bitrate_kbps > 0:
            headroom_pct = max(0, (uplink - self.current_bitrate_kbps) / self.current_bitrate_kbps * 100)
        else:
            headroom_pct = 0
        self.uplink_card.set_value(headroom_pct)
        self.uplink_card.set_grade(self._get_grade_for_headroom(headroom_pct))
        
        # System metrics
        cpu = metrics.get('cpu_pct', 0)
        self.cpu_card.set_value(cpu)
        self.cpu_card.set_grade(self._get_grade_for_cpu(cpu))
        
        gpu = metrics.get('gpu_pct', 0)
        self.gpu_card.set_value(gpu)
        self.gpu_card.set_grade(self._get_grade_for_gpu(gpu))
        
        mem = metrics.get('mem_mb', 0)
        self.mem_card.set_value(mem, "MB")
        self.mem_card.set_grade(self._get_grade_for_memory(mem))
        
        # OBS metrics (expert mode only)
        if not self.simple_mode:
            obs = metrics.get('obs', {})
            
            dropped = obs.get('dropped_ratio', 0) * 100
            self.dropped_card.set_value(dropped)
            self.dropped_card.set_grade(self._get_grade_for_dropped(dropped))
            
            enc_lag = obs.get('encoding_lag_ms', 0)
            self.enc_lag_card.set_value(enc_lag)
            self.enc_lag_card.set_grade(self._get_grade_for_enc_lag(enc_lag))
            
            render_lag = obs.get('render_lag_ms', 0)
            self.render_lag_card.set_value(render_lag)
            self.render_lag_card.set_grade(self._get_grade_for_render_lag(render_lag))
    
    def _update_quality_score(self):
        """품질 점수 업데이트"""
        # print(f"=== 품질 점수 디버그 ===")
        # print(f"metrics_window 크기 = {len(self.metrics_window)}")
        
        if not self.metrics_window:
            # print("metrics_window가 비어있음")
            return
        
        # 최근 메트릭 상세 출력
        # if self.metrics_window:
        #     latest = self.metrics_window[-1]
        #     print(f"최근 메트릭: {latest}")
        
        # OBS 설정 가져오기
        obs_settings = self.obs_manager.get_obs_settings()
        
        # Calculate quality score
        result = self.quality_score.compute_quality(self.metrics_window, self.current_bitrate_kbps, obs_settings)
        # print(f"품질 점수 계산 결과: {result}")
        
        # Update gauge
        self.quality_gauge.set_score(result['score'])
        self.quality_gauge.set_grade(result['grade'])
        self.quality_gauge.set_recommendation(result['action'])
        
        # Update recommendation
        self.recommendation_label.setText(result['action'])
        # print(f"=== 품질 점수 디버그 끝 ===")
    
    def _update_graphs(self, metrics: dict):
        """그래프 업데이트"""
        # print("=== 그래프 업데이트 시작 ===")
        current_time = time.time()
        
        # 네트워크 그래프 업데이트
        rtt_ms = metrics.get('rtt_ms', 0)
        loss_pct = metrics.get('loss_pct', 0)
        self.rtt_graph.add_point(current_time, rtt_ms)
        self.loss_graph.add_point(current_time, loss_pct)
        
        # 시스템 그래프 업데이트
        cpu_pct = metrics.get('cpu_pct', 0)
        gpu_pct = metrics.get('gpu_pct', 0)
        self.cpu_graph.add_point(current_time, cpu_pct)
        self.gpu_graph.add_point(current_time, gpu_pct)
        
        # OBS 그래프들 업데이트
        obs = metrics.get('obs', {})
        
        # OBS 메트릭이 없으면 시뮬레이션 데이터 사용
        if not obs:
            # 시간 기반 시뮬레이션 데이터
            import math
            t = current_time % 10  # 10초 주기
            dropped_ratio = 2.0 + math.sin(t) * 1.5  # 0.5~3.5%
            enc_lag = 5.0 + math.sin(t * 2) * 3.0    # 2~8ms
            render_lag = 3.0 + math.cos(t * 1.5) * 2.0  # 1~5ms
        else:
            # 실제 OBS 메트릭 사용
            dropped_ratio = obs.get('dropped_ratio', 0) * 100  # 퍼센트로 변환
            enc_lag = obs.get('encoding_lag_ms', 0)
            render_lag = obs.get('render_lag_ms', 0)
        
        # Dropped frames
        self.dropped_graph.add_point(current_time, dropped_ratio)
        
        # Encoding lag
        self.enc_lag_graph.add_point(current_time, enc_lag)
        
        # Render lag
        self.render_lag_graph.add_point(current_time, render_lag)
        
        # print(f"그래프 업데이트 완료: RTT={rtt_ms:.1f}ms, Loss={loss_pct:.1f}%, CPU={cpu_pct:.1f}%, GPU={gpu_pct:.1f}%")
        # print("=== 그래프 업데이트 끝 ===")
    
    def _toggle_monitoring(self):
        """실시간 모니터링 토글"""
        if not self.monitoring_active:
            # 모니터링 시작
            self.monitoring_active = True
            self.monitoring_btn.setText("모니터링 중지")
            self.monitoring_btn.setStyleSheet("""
                QPushButton {
                    background-color: #6c757d;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    font-size: 12px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #5a6268;
                }
            """)
            self._update_recommendation("실시간 모니터링이 활성화되었습니다. 메트릭이 실시간으로 업데이트됩니다.")
            self.debug_label.setText("디버그: 모니터링 시작됨")
            
            # OBS 폴링 시작
            if hasattr(self, 'obs_poller'):
                self.obs_poller.start_polling()
            
            # 그래프 활성화
            self._enable_graphs(True)
            
        else:
            # 모니터링 중지
            self.monitoring_active = False
            self.monitoring_btn.setText("모니터링 시작")
            self.monitoring_btn.setStyleSheet("""
                QPushButton {
                    background-color: #dc3545;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    font-size: 12px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #c82333;
                }
            """)
            self._update_recommendation("모니터링이 중지되었습니다. 시작 버튼을 눌러 실시간 모니터링을 시작하세요.")
            self.debug_label.setText("디버그: 모니터링 중지됨")
            
            # OBS 폴링 중지
            if hasattr(self, 'obs_poller'):
                self.obs_poller.stop_polling()
            
            # 그래프 비활성화
            self._enable_graphs(False)
    
    def _enable_graphs(self, enable: bool):
        """그래프 업데이트 활성화/비활성화"""
        # print(f"=== 그래프 활성화/비활성화: {enable} ===")
        
        # 시스템 메트릭 그래프
        graphs = [
            self.rtt_graph, self.loss_graph, self.cpu_graph, self.gpu_graph
        ]
        
        if not self.simple_mode:
            graphs.extend([self.dropped_graph, self.enc_lag_graph, self.render_lag_graph])
            # print(f"전문 모드: 총 {len(graphs)}개 그래프 처리")
        else:
            # print(f"간단 모드: 총 {len(graphs)}개 그래프 처리")
            pass # 간단 모드에서는 그래프 크기만 조정
        
        for i, graph in enumerate(graphs):
            if hasattr(graph, 'set_active'):
                # SimpleGraphWidget
                # print(f"그래프 {i}: SimpleGraphWidget.set_active({enable})")
                graph.set_active(enable)
        
        # print(f"=== 그래프 활성화/비활성화 완료: {enable} ===")
    
    def _update_recommendation(self, message: str):
        """권장 조치 메시지 업데이트"""
        self.recommendation_label.setText(message)
        
    def _on_backend_disconnected(self):
        """백엔드 연결 끊김 처리"""
        self.backend_status_label.setText("백엔드: 연결 끊김")
        self.backend_status_label.setStyleSheet("""
            QLabel {
                color: #ff4444;
                font-size: 11px;
                padding: 4px;
            }
        """)
        
    def _on_backend_connected(self):
        """백엔드 연결됨 처리"""
        self.backend_status_label.setText("백엔드: 연결됨")
        self.backend_status_label.setStyleSheet("""
            QLabel {
                color: #44ff44;
                font-size: 11px;
                padding: 4px;
            }
        """)
    
    def _toggle_mode(self, simple_mode: bool):
        """간단/전문 모드 토글"""
        self.simple_mode = simple_mode
        
        # Show/hide OBS cards and graphs
        self.dropped_card.setVisible(not simple_mode)
        self.enc_lag_card.setVisible(not simple_mode)
        self.render_lag_card.setVisible(not simple_mode)
        
        self.dropped_graph.setVisible(not simple_mode)
        self.enc_lag_graph.setVisible(not simple_mode)
        self.render_lag_graph.setVisible(not simple_mode)
        
        # 간단 모드에서는 그래프 크기 조정
        if simple_mode:
            # 그래프 높이를 줄임
            for graph in [self.rtt_graph, self.loss_graph, self.cpu_graph, self.gpu_graph]:
                graph.setMinimumHeight(80)
                graph.setMaximumHeight(120)
        else:
            # 전문 모드에서는 그래프 크기 복원
            for graph in [self.rtt_graph, self.loss_graph, self.cpu_graph, self.gpu_graph]:
                graph.setMinimumHeight(120)
                graph.setMaximumHeight(200)
    
    def _run_diagnostic(self):
        """진단 모드 실행"""
        # print("진단 모드 버튼 클릭됨")
        self.debug_label.setText("디버그: 진단 모드 버튼 클릭됨")
        try:
            from actions.diagnose import DiagnosticDialog
            # print("DiagnosticDialog 모듈 로드 성공")
            self.debug_label.setText("디버그: DiagnosticDialog 모듈 로드 성공")
            
            # 현재 선택된 플랫폼 가져오기
            platform_key = self.platform_combo.currentData()
            if not platform_key:
                platform_key = "soop"  # 기본값
            # print(f"선택된 플랫폼: {platform_key}")
            
            # 설정에서 진단 시간 가져오기
            diagnostic_minutes = self.config.get("diagnostic_duration_minutes", 60)
            diagnostic_seconds = diagnostic_minutes * 60
            
            # 진단 다이얼로그 실행 (설정값 사용)
            # print(f"진단 다이얼로그 시작... ({diagnostic_minutes}분)")
            self.debug_label.setText(f"디버그: 진단 다이얼로그 시작... ({diagnostic_minutes}분)")
            dialog = DiagnosticDialog(diagnostic_seconds, platform_key, self.metric_bus, self)
            dialog.exec()
            # print("진단 다이얼로그 완료")
            self.debug_label.setText("디버그: 진단 다이얼로그 완료")
            
        except ImportError as e:
            # print(f"진단 모드 모듈을 불러올 수 없습니다: {e}")
            self.debug_label.setText(f"디버그: ImportError - {e}")
        except Exception as e:
            # print(f"진단 모드 실행 중 오류: {e}")
            self.debug_label.setText(f"디버그: 오류 - {e}")
            import traceback
            traceback.print_exc()
    
    def _run_benchmark(self):
        """벤치마크 실행"""
        # print("벤치마크 버튼 클릭됨")
        self.debug_label.setText("디버그: 벤치마크 버튼 클릭됨")
        try:
            from actions.diagnose import DiagnosticDialog
            # print("DiagnosticDialog 모듈 로드 성공 (벤치마크)")
            self.debug_label.setText("디버그: DiagnosticDialog 모듈 로드 성공 (벤치마크)")
            
            # 현재 선택된 플랫폼 가져오기
            platform_key = self.platform_combo.currentData()
            if not platform_key:
                platform_key = "soop"  # 기본값
            # print(f"선택된 플랫폼 (벤치마크): {platform_key}")
            
            # 벤치마크 다이얼로그 실행 (10초)
            # print("벤치마크 다이얼로그 시작...")
            self.debug_label.setText("디버그: 벤치마크 다이얼로그 시작...")
            dialog = DiagnosticDialog(10, platform_key, self.metric_bus, self)
            dialog.exec()
            # print("벤치마크 다이얼로그 완료")
            self.debug_label.setText("디버그: 벤치마크 다이얼로그 완료")
            
        except ImportError as e:
            # print(f"벤치마크 모듈을 불러올 수 없습니다: {e}")
            self.debug_label.setText(f"디버그: ImportError (벤치마크) - {e}")
        except Exception as e:
            # print(f"벤치마크 실행 중 오류: {e}")
            self.debug_label.setText(f"디버그: 오류 (벤치마크) - {e}")
            import traceback
            traceback.print_exc()
    
    def _show_help(self):
        """도움말 다이얼로그 표시"""
        try:
            from widgets.help_dialog import HelpDialog
            dialog = HelpDialog(self)
            dialog.exec()
        except ImportError as e:
            self.debug_label.setText(f"디버그: 도움말 모듈 로드 실패 - {e}")
        except Exception as e:
            self.debug_label.setText(f"디버그: 도움말 오류 - {e}")
            import traceback
            traceback.print_exc()
    
    def _on_stream_interruption(self, message: str):
        """스트림 끊김 알림 처리"""
        self.debug_label.setText(f"🚨 {message}")
        # 여기에 추가 알림 로직 (예: 팝업, 사운드 등) 추가 가능
    
    def _on_quality_degradation(self, message: str):
        """품질 저하 알림 처리"""
        self.debug_label.setText(f"⚠️ {message}")
        # 여기에 추가 알림 로직 추가 가능
    
    def _on_stream_recovered(self):
        """스트림 복구 알림 처리"""
        self.debug_label.setText("✅ 스트리밍이 정상으로 복구되었습니다.")
        # 여기에 추가 알림 로직 추가 가능
    
    # Grade calculation helpers
    def _get_grade_for_rtt(self, rtt: float) -> str:
        if rtt <= 50: return "좋음"
        elif rtt <= 100: return "주의"
        else: return "불안정"
    
    def _get_grade_for_loss(self, loss: float) -> str:
        if loss <= 0.5: return "좋음"
        elif loss <= 2: return "주의"
        else: return "불안정"
    
    def _get_grade_for_headroom(self, headroom: float) -> str:
        if headroom >= 50: return "좋음"
        elif headroom >= 20: return "주의"
        else: return "불안정"
    
    def _get_grade_for_cpu(self, cpu: float) -> str:
        if cpu <= 50: return "좋음"
        elif cpu <= 80: return "주의"
        else: return "불안정"
    
    def _get_grade_for_gpu(self, gpu: float) -> str:
        if gpu <= 60: return "좋음"
        elif gpu <= 85: return "주의"
        else: return "불안정"
    
    def _get_grade_for_memory(self, mem_mb: float) -> str:
        # Assuming 16GB system
        usage_pct = (mem_mb / 16384) * 100
        if usage_pct <= 70: return "좋음"
        elif usage_pct <= 90: return "주의"
        else: return "불안정"
    
    def _get_grade_for_dropped(self, dropped_pct: float) -> str:
        if dropped_pct <= 1: return "좋음"
        elif dropped_pct <= 3: return "주의"
        else: return "불안정"
    
    def _get_grade_for_enc_lag(self, lag_ms: float) -> str:
        if lag_ms <= 5: return "좋음"
        elif lag_ms <= 15: return "주의"
        else: return "불안정"
    
    def _get_grade_for_render_lag(self, lag_ms: float) -> str:
        if lag_ms <= 7: return "좋음"
        elif lag_ms <= 20: return "주의"
        else: return "불안정"
    
    def set_current_bitrate(self, bitrate_kbps: int):
        """현재 비트레이트 설정"""
        self.current_bitrate_kbps = bitrate_kbps
    
    def _on_platform_changed(self, platform_name: str):
        """플랫폼 변경 시 호출"""
        # 플랫폼 키 찾기
        platform_key = None
        platform_names = get_platform_display_names()
        for key, name in platform_names.items():
            if name == platform_name:
                platform_key = key
                break
        
        if platform_key and self.metrics_window:
            # 최근 메트릭으로 권장 설정 계산
            recent_metrics = self.metrics_window[-1] if self.metrics_window else {}
            net_metrics = recent_metrics.get('net', {})
            
            uplink_kbps = net_metrics.get('uplink_kbps', 10000)
            rtt_ms = net_metrics.get('rtt_ms', 50)
            loss_pct = net_metrics.get('loss_pct', 0.5)
            
            # 권장 설정 계산
            recommended = get_recommended_settings(platform_key, uplink_kbps, rtt_ms, loss_pct)
            
            # 권장 설정 표시
            self._show_recommended_settings(recommended)
    
    def _show_recommended_settings(self, settings: dict):
        """권장 설정 표시"""
        # 하단 권장사항 업데이트
        recommendation_text = f"""
        💡 {settings['platform']} 권장 설정:
        해상도: {settings['resolution']} @ {settings['fps']}fps
        비트레이트: {settings['video_bitrate_kbps']} kbps
        키프레임: {settings['keyframe_interval_sec']}초
        인코더: {settings['encoder_hint']}
        
        근거: {settings['rationale']}
        """
        
        self.recommendation_label.setText(recommendation_text.strip())
    
    def _on_obs_connected(self):
        """OBS 연결됨"""
        # print("=== OBS 연결됨 ===")
        self.debug_label.setText("디버그: OBS 연결됨 ✅ (메트릭 수집 중...)")
        # print("OBS WebSocket 연결 성공")
        # print("OBS 메트릭 수신 대기 중...")
        
        # OBS 설정 가져오기 (폴러에서 직접)
        try:
            if hasattr(self, 'obs_poller') and self.obs_poller:
                obs_settings = self.obs_poller.get_obs_settings()
                print(f"OBS 설정 조회: {obs_settings}")
                self.obs_settings_display.update_settings(obs_settings)
            else:
                print("OBS 폴러가 없음")
                obs_settings = self.obs_manager.get_obs_settings()
                self.obs_settings_display.update_settings(obs_settings)
            print(f"OBS 설정 로드됨: {obs_settings}")
        except Exception as e:
            print(f"OBS 설정 로드 실패: {e}")
    
    def _on_obs_disconnected(self):
        """OBS 연결 해제됨"""
        # print("=== OBS 연결 끊김 ===")
        self.debug_label.setText("디버그: OBS 연결 끊김 ❌ (재연결 시도 중...)")
        # print("OBS WebSocket 연결 끊김")
        # print("메트릭 수집 루프에서 오류가 발생했을 수 있습니다.")
        
        # OBS 메트릭 초기화
        if not self.simple_mode:
            self.dropped_card.set_value(0)
            self.enc_lag_card.set_value(0)
            self.render_lag_card.set_value(0)
    
    def _on_obs_metrics_updated(self, metrics: dict):
        """OBS 메트릭 업데이트"""
        print(f"=== OBS 메트릭 수신됨 ===")
        print(f"수신된 메트릭: {metrics}")
        
        # 메트릭이 비어있거나 None인지 확인
        if not metrics:
            print("메트릭이 비어있음")
            return
        
        # 현재 시간
        now = time.time()
        
        # PyQtGraph 위젯에 데이터 추가
        if hasattr(self, 'dropped_graph'):
            dropped_ratio = metrics.get('dropped_ratio', 0.0) * 100  # 퍼센트로 변환
            self.dropped_graph.add_point(now, dropped_ratio)
            print(f"그래프 업데이트: dropped_ratio = {dropped_ratio:.2f}%")
        
        if hasattr(self, 'enc_lag_graph'):
            encoding_lag = metrics.get('encoding_lag_ms', 0.0)
            self.enc_lag_graph.add_point(now, encoding_lag)
            print(f"그래프 업데이트: encoding_lag = {encoding_lag:.2f}ms")
        
        if hasattr(self, 'render_lag_graph'):
            render_lag = metrics.get('render_lag_ms', 0.0)
            self.render_lag_graph.add_point(now, render_lag)
            print(f"그래프 업데이트: render_lag = {render_lag:.2f}ms")
        
        # 상태 카드 업데이트 - 항상 강제 업데이트
        dropped_ratio = metrics.get('dropped_ratio', 0.0) * 100
        encoding_lag = metrics.get('encoding_lag_ms', 0.0)
        render_lag = metrics.get('render_lag_ms', 0.0)
        
        print(f"상태 카드 업데이트: dropped={dropped_ratio:.2f}%, enc_lag={encoding_lag:.2f}ms, render_lag={render_lag:.2f}ms")
        
        # 메트릭 값이 실제로 변경되었는지 확인
        if dropped_ratio == 0.0 and encoding_lag == 0.0 and render_lag == 0.0:
            print("모든 메트릭이 0.0 - 실제 데이터가 아닐 수 있음")
        else:
            print(f"실제 메트릭 값 감지: dropped={dropped_ratio:.2f}%, enc={encoding_lag:.2f}ms, render={render_lag:.2f}ms")
        
        # 강제로 상태 카드 업데이트
        try:
            if hasattr(self, 'dropped_card') and self.dropped_card:
                self.dropped_card.set_value(dropped_ratio)
                # 등급도 함께 업데이트
                if dropped_ratio < 1.0:
                    self.dropped_card.set_grade("좋음")
                elif dropped_ratio < 3.0:
                    self.dropped_card.set_grade("주의")
                else:
                    self.dropped_card.set_grade("불안정")
                self.dropped_card.repaint()  # 강제 리페인트
                self.dropped_card.update()  # 추가 업데이트
                print(f"dropped_card 업데이트 완료: {dropped_ratio:.2f}%")
            else:
                print("dropped_card가 존재하지 않음")
        except Exception as e:
            print(f"dropped_card 업데이트 오류: {e}")
        
        try:
            if hasattr(self, 'enc_lag_card') and self.enc_lag_card:
                self.enc_lag_card.set_value(encoding_lag)
                # 등급도 함께 업데이트
                if encoding_lag < 10:
                    self.enc_lag_card.set_grade("좋음")
                elif encoding_lag < 20:
                    self.enc_lag_card.set_grade("주의")
                else:
                    self.enc_lag_card.set_grade("불안정")
                self.enc_lag_card.repaint()  # 강제 리페인트
                self.enc_lag_card.update()  # 추가 업데이트
                print(f"enc_lag_card 업데이트 완료: {encoding_lag:.2f}%")
            else:
                print("enc_lag_card가 존재하지 않음")
        except Exception as e:
            print(f"enc_lag_card 업데이트 오류: {e}")
        
        try:
            if hasattr(self, 'render_lag_card') and self.render_lag_card:
                self.render_lag_card.set_value(render_lag)
                # 등급도 함께 업데이트
                if render_lag < 14:
                    self.render_lag_card.set_grade("좋음")
                elif render_lag < 25:
                    self.render_lag_card.set_grade("주의")
                else:
                    self.render_lag_card.set_grade("불안정")
                self.render_lag_card.repaint()  # 강제 리페인트
                self.render_lag_card.update()  # 추가 업데이트
                print(f"render_lag_card 업데이트 완료: {render_lag:.2f}ms")
            else:
                print("render_lag_card가 존재하지 않음")
        except Exception as e:
            print(f"render_lag_card 업데이트 오류: {e}")
        
        # 연결 상태 업데이트
        self.debug_label.setText("디버그: OBS 연결됨 ✅ (메트릭 수신 중)")
        
        print(f"=== OBS 메트릭 처리 완료 ===")
    
    def _open_settings(self):
        """통합 설정 다이얼로그 열기"""
        try:
            # 디버그 메시지 추가
            self.debug_label.setText("디버그: 설정 다이얼로그 열기 시도 중...")
            # print("설정 버튼 클릭됨 - 다이얼로그 열기 시도")
            
            current_config = load()
            
            # print(f"현재 설정 로드됨: {current_config}")
            
            dialog = IntegratedSettingsDialog(current_config, self)
            
            # print("다이얼로그 표시 중...")
            result = dialog.exec()
            # print(f"다이얼로그 결과: {result}")
            
            if result == IntegratedSettingsDialog.Accepted:
                # 설정값 저장
                new_settings = dialog.get_settings()
                # print(f"새 설정값: {new_settings}")
                
                # 설정 파일에 저장
                save(new_settings)
                
                # 백엔드 경로 업데이트
                if new_settings.get('backend_path') and new_settings['backend_path'] != self.config.get('backend_path'):
                    self.config['backend_path'] = new_settings['backend_path']
                    # 메트릭 버스 재시작
                    if hasattr(self.metric_bus, 'stop'):
                        self.metric_bus.stop()
                    if hasattr(self.metric_bus, 'start'):
                        self.metric_bus.start()
                
                # OBS 클라이언트 재설정
                if hasattr(self.metric_bus, 'reconfigure_obs_client'):
                    # print("메트릭 버스를 통해 OBS 클라이언트 재설정")
                    obs_config = new_settings.get('obs', {})
                    self.metric_bus.reconfigure_obs_client(
                        host=obs_config.get('host', '127.0.0.1'),
                        port=obs_config.get('port', 4455),
                        password=obs_config.get('password', ''),
                        use_tls=obs_config.get('use_tls', False)
                    )
                else:
                    # print("메트릭 버스에 reconfigure_obs_client 메서드 없음")
                    # 기존 방식으로 OBS 클라이언트 재시작
                    if hasattr(self.obs_manager, 'stop_poller'): # Changed from obs_client to obs_manager
                        self.obs_manager.stop_poller()
                    
                    # 새로운 설정으로 OBS 클라이언트 재생성
                    from core.obs_client_manager import ObsClientManager
                    obs_config = new_settings.get('obs', {})
                    self.obs_manager = ObsClientManager() # Changed from obs_client to obs_manager
                    self.obs_poller = self.obs_manager.create_poller(
                        host=obs_config.get('host', '127.0.0.1'),
                        port=obs_config.get('port', 4455),
                        password=obs_config.get('password', ''),
                        use_tls=obs_config.get('use_tls', False)
                    )
                    self.obs_poller.tick.connect(self._on_obs_metrics_updated)
                    self.obs_poller.connected.connect(self._on_obs_connected)
                    self.obs_poller.disconnected.connect(self._on_obs_disconnected)
                    
                    # OBS 연결 시작
                    self.obs_poller.start() # Changed from obs_client.start() to obs_poller.start()
                
                # 설정 업데이트
                self.config.update(new_settings)
                
                # print(f"설정 업데이트 완료: {new_settings}")
                self.debug_label.setText("디버그: 설정 업데이트 완료")
                
        except Exception as e:
            error_msg = f"설정 다이얼로그 오류: {e}"
            # print(error_msg)
            self.debug_label.setText(f"디버그: {error_msg}")
            import traceback
            traceback.print_exc()
    
    def _update_obs_metrics(self, metrics: dict):
        """OBS 메트릭을 UI에 반영"""
        # print(f"=== OBS 메트릭 업데이트 디버그 ===")
        # print(f"받은 OBS 메트릭: {metrics}")
        
        # 드롭된 프레임 비율
        dropped_ratio = metrics.get('dropped_ratio', 0) * 100
        # print(f"드롭된 프레임 비율: {dropped_ratio:.1f}%")
        self.dropped_card.update_value(dropped_ratio, f"{dropped_ratio:.1f}%")
        self.dropped_card.update_status(self._get_grade_for_dropped(dropped_ratio))
        
        # 인코딩 지연
        enc_lag = metrics.get('encoding_lag_ms', 0)
        # print(f"인코딩 지연: {enc_lag:.1f}ms")
        self.enc_lag_card.update_value(enc_lag, f"{enc_lag:.1f}ms")
        self.enc_lag_card.update_status(self._get_grade_for_enc_lag(enc_lag))
        
        # 렌더링 지연
        render_lag = metrics.get('render_lag_ms', 0)
        # print(f"렌더링 지연: {render_lag:.1f}ms")
        self.render_lag_card.update_value(render_lag, f"{render_lag:.1f}ms")
        self.render_lag_card.update_status(self._get_grade_for_render_lag(render_lag))
        
        # print(f"=== OBS 메트릭 업데이트 디버그 끝 ===")
    
    def closeEvent(self, event):
        """윈도우 종료 시 정리 작업"""
        try:
            # print("대시보드 종료 중...")
            if hasattr(self, 'metric_bus') and self.metric_bus:
                try:
                    self.metric_bus.stop()
                except Exception as e:
                    # print(f"메트릭 버스 종료 오류: {e}")
                    pass
            
            if hasattr(self, 'obs_manager') and self.obs_manager: # Changed from obs_client to obs_manager
                try:
                    self.obs_manager.stop_poller() # Changed from obs_client.stop() to obs_manager.stop_poller()
                except Exception as e:
                    # print(f"OBS 클라이언트 종료 오류: {e}")
                    pass
            
            # print("대시보드 종료 완료")
            event.accept()
        except Exception as e:
            # print(f"대시보드 종료 중 오류: {e}")
            import traceback
            traceback.print_exc()
            event.accept()
    
    def resizeEvent(self, event):
        """윈도우 크기 변경 시 안전하게 처리"""
        try:
            super().resizeEvent(event)
        except Exception as e:
            # print(f"윈도우 크기 변경 오류: {e}")
            # 오류가 발생해도 기본 동작 수행
            event.accept()
    
    def moveEvent(self, event):
        """윈도우 이동 시 안전하게 처리"""
        try:
            super().moveEvent(event)
        except Exception as e:
            # print(f"윈도우 이동 오류: {e}")
            # 오류가 발생해도 기본 동작 수행
            event.accept()
