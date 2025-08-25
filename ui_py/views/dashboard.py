from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, 
                               QLabel, QPushButton, QCheckBox, QSplitter, QFrame,
                               QComboBox, QGroupBox)
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont

from widgets.status_card import StatusCard
from widgets.gauge import GaugeWidget
from widgets.metric_graph import MetricGraph
from core.metric_bus import MetricBus
from core.score import QualityScore
from platform_rules import get_platform_list, get_platform_display_names, get_recommended_settings
from core.obs_client import ObsClient

class DashboardView(QWidget):
    """메인 대시보드 뷰"""
    
    def __init__(self, metric_bus: MetricBus, parent=None):
        super().__init__(parent)
        self.metric_bus = metric_bus
        self.quality_score = QualityScore()
        self.current_bitrate_kbps = 6000  # Default
        self.simple_mode = False
        self.metrics_window = []  # Recent metrics for scoring
        self.monitoring_active = False  # 모니터링 활성화 상태
        
        # OBS 클라이언트 초기화 (콜백 방식)
        self.obs_client = ObsClient(self)
        self.obs_client.set_connected_callback(self._on_obs_connected)
        self.obs_client.set_disconnected_callback(self._on_obs_disconnected)
        self.obs_client.set_metrics_callback(self._on_obs_metrics_updated)
        
        self._setup_ui()
        self._setup_connections()
        self._apply_dark_theme()
        
        # OBS 연결 시작
        self.obs_client.start_connection()
        
        # 초기 권장 조치 메시지 설정
        self._update_recommendation("실시간 모니터링을 시작하려면 상단의 빨간 버튼을 클릭하세요.")
        
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
        diagnostic_btn = QPushButton("진단 모드 (60초)")
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
        # 모니터링이 활성화된 경우에만 업데이트
        if not self.monitoring_active:
            return
            
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
        else:
            # 모니터링 중지
            self.monitoring_active = False
            self.monitoring_btn.setText("실시간 모니터링 시작")
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
            self._update_recommendation("모니터링이 중지되었습니다. 다시 시작하려면 버튼을 클릭하세요.")
    
    def _update_recommendation(self, message: str):
        """권장 조치 메시지 업데이트"""
        self.recommendation_label.setText(message)
    
    def _toggle_mode(self, simple_mode: bool):
        """간단/전문 모드 토글"""
        self.simple_mode = simple_mode
        
        # Show/hide OBS cards and graphs
        self.dropped_card.setVisible(not simple_mode)
        self.enc_lag_card.setVisible(not simple_mode)
        self.render_lag_card.setVisible(not simple_mode)
        
        self.dropped_graph.setVisible(not simple_mode)
        self.lag_graph.setVisible(not simple_mode)
        
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
        try:
            from actions.diagnose import DiagnosticDialog
            
            # 현재 선택된 플랫폼 가져오기
            platform_key = self.platform_combo.currentData()
            if not platform_key:
                platform_key = "soop"  # 기본값
            
            # 진단 다이얼로그 실행 (60초)
            dialog = DiagnosticDialog(60, platform_key, self.metric_bus, self)
            dialog.exec()
            
        except ImportError as e:
            print(f"진단 모드 모듈을 불러올 수 없습니다: {e}")
        except Exception as e:
            print(f"진단 모드 실행 중 오류: {e}")
    
    def _run_benchmark(self):
        """벤치마크 실행"""
        try:
            from actions.diagnose import DiagnosticDialog
            
            # 현재 선택된 플랫폼 가져오기
            platform_key = self.platform_combo.currentData()
            if not platform_key:
                platform_key = "soop"  # 기본값
            
            # 벤치마크 다이얼로그 실행 (10초)
            dialog = DiagnosticDialog(10, platform_key, self.metric_bus, self)
            dialog.exec()
            
        except ImportError as e:
            print(f"벤치마크 모듈을 불러올 수 없습니다: {e}")
        except Exception as e:
            print(f"벤치마크 실행 중 오류: {e}")
    
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
        print("OBS 연결됨")
        # OBS 연결 상태를 UI에 반영할 수 있음
    
    def _on_obs_disconnected(self):
        """OBS 연결 해제됨"""
        print("OBS 연결 해제됨")
        # OBS 연결 해제 상태를 UI에 반영할 수 있음
    
    def _on_obs_metrics_updated(self, metrics: dict):
        """OBS 메트릭 업데이트"""
        # OBS 메트릭을 메트릭 버스에 전달
        if hasattr(self.metric_bus, 'update_obs_metrics'):
            self.metric_bus.update_obs_metrics(metrics)
        
        # OBS 메트릭을 UI에 반영
        self._update_obs_metrics(metrics)
    
    def _update_obs_metrics(self, metrics: dict):
        """OBS 메트릭을 UI에 반영"""
        # 드롭된 프레임 비율
        dropped_ratio = metrics.get('dropped_ratio', 0) * 100
        self.dropped_card.update_value(dropped_ratio, f"{dropped_ratio:.1f}%")
        self.dropped_card.update_status(self._get_grade_for_dropped(dropped_ratio))
        
        # 인코딩 지연
        enc_lag = metrics.get('encoding_lag_ms', 0)
        self.enc_lag_card.update_value(enc_lag, f"{enc_lag:.1f}ms")
        self.enc_lag_card.update_status(self._get_grade_for_enc_lag(enc_lag))
        
        # 렌더링 지연
        render_lag = metrics.get('render_lag_ms', 0)
        self.render_lag_card.update_value(render_lag, f"{render_lag:.1f}ms")
        self.render_lag_card.update_status(self._get_grade_for_render_lag(render_lag))
