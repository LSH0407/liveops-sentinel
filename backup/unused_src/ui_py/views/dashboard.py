from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, 
                               QLabel, QPushButton, QCheckBox, QSplitter, QFrame)
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont

from widgets.status_card import StatusCard
from widgets.gauge import GaugeWidget
from widgets.metric_graph import MetricGraph
from core.metric_bus import MetricBus
from core.score import QualityScore

class DashboardView(QWidget):
    """메인 대시보드 뷰"""
    
    def __init__(self, metric_bus: MetricBus, parent=None):
        super().__init__(parent)
        self.metric_bus = metric_bus
        self.quality_score = QualityScore()
        self.current_bitrate_kbps = 6000  # Default
        self.simple_mode = False
        self.metrics_window = []  # Recent metrics for scoring
        
        self._setup_ui()
        self._setup_connections()
        self._apply_dark_theme()
    
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
        layout.addWidget(splitter)
        
        # Bottom bar: Recommendations
        bottom_bar = self._create_bottom_bar()
        layout.addWidget(bottom_bar)
    
    def _create_top_bar(self) -> QWidget:
        """상단 컨트롤 바 생성"""
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        
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
        
        layout.addStretch()
        
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
        self.dropped_card = StatusCard("버린 프레임", "%")
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
        
        # Network graphs
        net_layout = QHBoxLayout()
        self.rtt_graph = MetricGraph("서버 응답 속도 (RTT)")
        self.loss_graph = MetricGraph("전송 손실")
        net_layout.addWidget(self.rtt_graph)
        net_layout.addWidget(self.loss_graph)
        layout.addLayout(net_layout)
        
        # System graphs
        sys_layout = QHBoxLayout()
        self.cpu_graph = MetricGraph("CPU 사용률")
        self.gpu_graph = MetricGraph("GPU 사용률")
        sys_layout.addWidget(self.cpu_graph)
        sys_layout.addWidget(self.gpu_graph)
        layout.addLayout(sys_layout)
        
        # OBS graphs (expert mode only)
        obs_layout = QHBoxLayout()
        self.dropped_graph = MetricGraph("버린 프레임 비율")
        self.lag_graph = MetricGraph("인코딩/렌더 지연")
        obs_layout.addWidget(self.dropped_graph)
        obs_layout.addWidget(self.lag_graph)
        layout.addLayout(obs_layout)
        
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
        self.metric_bus.subscribe(self._on_metrics_update)
        
        # Setup graphs
        self._setup_graphs()
    
    def _setup_graphs(self):
        """그래프 설정"""
        # Network graphs
        self.rtt_graph.set_series(self.metric_bus, "net.rtt_ms", "ms", "#00ff00")
        self.loss_graph.set_series(self.metric_bus, "net.loss_pct", "%", "#ffaa00")
        
        # System graphs
        self.cpu_graph.set_series(self.metric_bus, "sys.cpu_pct", "%", "#ff0000")
        self.gpu_graph.set_series(self.metric_bus, "sys.gpu_pct", "%", "#0088ff")
        
        # OBS graphs
        self.dropped_graph.set_series(self.metric_bus, "obs.dropped_ratio", "%", "#ff00ff")
        self.lag_graph.set_series(self.metric_bus, "obs.enc_lag_ms", "ms", "#00ffff")
    
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
        # Store in window for scoring
        self.metrics_window.append(metrics)
        if len(self.metrics_window) > 50:  # Keep last 50 samples
            self.metrics_window.pop(0)
        
        # Update KPI cards
        self._update_kpi_cards(metrics)
        
        # Update quality score
        self._update_quality_score()
    
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
        if not self.metrics_window:
            return
        
        # Calculate quality score
        result = self.quality_score.compute_quality(self.metrics_window, self.current_bitrate_kbps)
        
        # Update gauge
        self.quality_gauge.set_score(result['score'])
        self.quality_gauge.set_grade(result['grade'])
        self.quality_gauge.set_recommendation(result['action'])
        
        # Update recommendation
        self.recommendation_label.setText(result['action'])
    
    def _toggle_mode(self, simple_mode: bool):
        """간단/전문 모드 토글"""
        self.simple_mode = simple_mode
        
        # Show/hide OBS cards and graphs
        self.dropped_card.setVisible(not simple_mode)
        self.enc_lag_card.setVisible(not simple_mode)
        self.render_lag_card.setVisible(not simple_mode)
        
        self.dropped_graph.setVisible(not simple_mode)
        self.lag_graph.setVisible(not simple_mode)
    
    def _run_benchmark(self):
        """벤치마크 실행"""
        # TODO: Implement benchmark
        print("벤치마크 실행...")
    
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
