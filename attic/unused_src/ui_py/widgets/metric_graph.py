from PySide6.QtCore import QTimer, Qt
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel
import pyqtgraph as pg
import time
from typing import List, Tuple, Optional, Dict

class MetricGraph(QWidget):
    """작업관리자 스타일 메트릭 그래프"""
    
    def __init__(self, title: str = "", parent=None):
        super().__init__(parent)
        self.title = title
        self.metric_bus = None
        self.metric_key = ""
        self.units = ""
        self.window_seconds = 60
        self.update_rate_hz = 4
        self.fill_under = True
        self.color = None
        self.markers = []  # [(timestamp, text), ...]
        
        self._setup_ui()
        self._setup_timer()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        
        # Title
        if self.title:
            title_label = QLabel(self.title)
            title_label.setStyleSheet("font-weight: bold; font-size: 12px; color: #e0e0e0;")
            layout.addWidget(title_label)
        
        # Graph
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('k')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.plot_widget.setLabel('left', '')
        self.plot_widget.setLabel('bottom', '')
        self.plot_widget.getAxis('left').setTextPen('w')
        self.plot_widget.getAxis('bottom').setTextPen('w')
        self.plot_widget.getAxis('left').setPen('w')
        self.plot_widget.getAxis('bottom').setPen('w')
        
        # Enable downsampling for performance
        self.plot_widget.setDownsampling(auto=True, mode='peak')
        self.plot_widget.setClipToView(True)
        self.plot_widget.setMouseEnabled(x=False, y=False)
        
        layout.addWidget(self.plot_widget)
        
        # Data curve
        self.curve = self.plot_widget.plot(pen=None)
        self.fill_curve = None
        
        # Marker lines
        self.marker_lines = []
    
    def _setup_timer(self):
        """업데이트 타이머 설정"""
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self._update_graph)
        self.update_timer.start(1000 // self.update_rate_hz)  # Convert Hz to ms
    
    def set_series(self, metric_bus, key: str, units: str = "", color: str = None):
        """메트릭 시리즈 설정"""
        self.metric_bus = metric_bus
        self.metric_key = key
        self.units = units
        
        if color:
            self.color = color
            pen = pg.mkPen(color=color, width=2)
            self.curve.setPen(pen)
            
            if self.fill_under:
                brush = pg.mkBrush(color=color, alpha=0.3)
                self.fill_curve = self.plot_widget.plot(fillLevel=0, brush=brush)
        else:
            # Default colors
            colors = ['#00ff00', '#ffaa00', '#ff0000', '#0088ff', '#ff00ff', '#00ffff']
            color = colors[hash(key) % len(colors)]
            pen = pg.mkPen(color=color, width=2)
            self.curve.setPen(pen)
            
            if self.fill_under:
                brush = pg.mkBrush(color=color, alpha=0.3)
                self.fill_curve = self.plot_widget.plot(fillLevel=0, brush=brush)
        
        # Set Y axis label
        if units:
            self.plot_widget.setLabel('left', units)
    
    def set_window(self, seconds: int = 60):
        """시간 윈도우 설정"""
        self.window_seconds = seconds
    
    def set_update_rate(self, hz: int = 4):
        """업데이트 주기 설정"""
        self.update_rate_hz = hz
        self.update_timer.start(1000 // hz)
    
    def add_marker(self, timestamp: float, text: str = ""):
        """알림 마커 추가"""
        self.markers.append((timestamp, text))
        self._update_markers()
    
    def clear_markers(self):
        """마커 제거"""
        self.markers.clear()
        self._update_markers()
    
    def _update_graph(self):
        """그래프 업데이트"""
        if not self.metric_bus or not self.metric_key:
            return
        
        # Get time series data
        series = self.metric_bus.series(self.metric_key)
        if not series:
            return
        
        # Filter to window
        now = time.time()
        window_data = [(ts, val) for ts, val in series if now - ts <= self.window_seconds]
        
        if not window_data:
            return
        
        # Downsample for performance (4Hz render)
        if len(window_data) > 240:  # 60s * 4Hz = 240 points
            step = len(window_data) // 240
            window_data = window_data[::step]
        
        # Extract x, y values
        timestamps, values = zip(*window_data)
        
        # Convert to relative time (seconds ago)
        x_data = [now - ts for ts in timestamps]
        y_data = [float(val) for val in values]
        
        # Update curves
        self.curve.setData(x_data, y_data)
        
        if self.fill_curve:
            self.fill_curve.setData(x_data, y_data)
        
        # Update X axis range
        self.plot_widget.setXRange(0, self.window_seconds)
        
        # Auto-scale Y axis
        if y_data:
            min_val = min(y_data)
            max_val = max(y_data)
            margin = (max_val - min_val) * 0.1
            if margin == 0:
                margin = 1
            self.plot_widget.setYRange(min_val - margin, max_val + margin)
    
    def _update_markers(self):
        """마커 라인 업데이트"""
        # Clear existing markers
        for line in self.marker_lines:
            self.plot_widget.removeItem(line)
        self.marker_lines.clear()
        
        # Add new markers
        now = time.time()
        for timestamp, text in self.markers:
            if now - timestamp <= self.window_seconds:
                x_pos = now - timestamp
                line = pg.InfiniteLine(pos=x_pos, angle=90, pen=pg.mkPen('r', width=2))
                self.plot_widget.addItem(line)
                self.marker_lines.append(line)
                
                if text:
                    # Add text label
                    text_item = pg.TextItem(text=text, color='r')
                    text_item.setPos(x_pos, self.plot_widget.getViewBox().viewRange()[1][1])
                    self.plot_widget.addItem(text_item)
                    self.marker_lines.append(text_item)
    
    def set_title(self, title: str):
        """제목 설정"""
        self.title = title
        if hasattr(self, 'title_label'):
            self.title_label.setText(title)
    
    def set_color(self, color: str):
        """색상 설정"""
        self.color = color
        pen = pg.mkPen(color=color, width=2)
        self.curve.setPen(pen)
        
        if self.fill_curve:
            brush = pg.mkBrush(color=color, alpha=0.3)
            self.fill_curve.setBrush(brush)
