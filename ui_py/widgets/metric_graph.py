from PySide6.QtCore import QTimer, Qt, QPointF
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QFrame
from PySide6.QtGui import QPainter, QPen, QBrush, QColor, QPolygonF, QFont
import time
from typing import List, Tuple, Optional, Dict

class SimpleGraphWidget(QFrame):
    """간단한 그래프 위젯 (numpy 없이)"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(100)
        self.data_points = []  # [(timestamp, value), ...]
        self.window_seconds = 60
        self.color = QColor(0, 255, 0)
        self.fill_under = True
        
    def set_data(self, data_points: List[Tuple[float, float]]):
        """데이터 설정"""
        self.data_points = data_points
        self.update()
    
    def set_color(self, color: QColor):
        """색상 설정"""
        self.color = color
        self.update()
    
    def set_window(self, seconds: int):
        """시간 윈도우 설정"""
        self.window_seconds = seconds
        self.update()
    
    def paintEvent(self, event):
        """그래프 그리기"""
        try:
            if not self.data_points:
                return
                
            painter = QPainter(self)
            painter.setRenderHint(QPainter.Antialiasing)
            
            # 배경
            painter.fillRect(self.rect(), QColor(0, 0, 0))
            
            # 데이터 필터링 (시간 윈도우)
            now = time.time()
            window_data = [(ts, val) for ts, val in self.data_points if now - ts <= self.window_seconds]
            
            if not window_data:
                return
            
            # 좌표 변환
            width = self.width()
            height = self.height()
            
            if width <= 0 or height <= 0:
                return
            
            # Y 범위 계산
            values = [val for _, val in window_data]
            if not values:
                return
                
            min_val = min(values)
            max_val = max(values)
            if min_val == max_val:
                min_val -= 1
                max_val += 1
            
            # X 범위
            x_min = now - self.window_seconds
            x_max = now
            
            # 포인트 변환
            points = []
            for ts, val in window_data:
                try:
                    x = width * (ts - x_min) / (x_max - x_min)
                    y = height * (1.0 - (val - min_val) / (max_val - min_val))
                    points.append(QPointF(x, y))
                except (ZeroDivisionError, ValueError):
                    continue
            
            if len(points) < 2:
                return
            
            # 선 그리기
            pen = QPen(self.color, 2)
            painter.setPen(pen)
            
            # 선 그리기
            for i in range(len(points) - 1):
                painter.drawLine(points[i], points[i + 1])
            
            # 채우기
            if self.fill_under and len(points) >= 2:
                fill_color = QColor(self.color)
                fill_color.setAlpha(80)
                painter.setBrush(QBrush(fill_color))
                
                polygon = QPolygonF(points)
                polygon.append(QPointF(points[-1].x(), height))
                polygon.append(QPointF(points[0].x(), height))
                
                painter.drawPolygon(polygon)
                
        except Exception as e:
            print(f"그래프 그리기 오류: {e}")
            import traceback
            traceback.print_exc()
        
        # Y축 최대값 표시
        painter.setPen(QPen(QColor(128, 128, 128), 1))
        painter.setFont(QFont("Arial", 8))
        
        # 최대값 텍스트
        max_text = f"{max_val:.1f}"
        if hasattr(self, 'units') and self.units:
            max_text += f" {self.units}"
        
        # 텍스트 위치 계산 (우상단)
        text_rect = painter.fontMetrics().boundingRect(max_text)
        text_x = width - text_rect.width() - 5
        text_y = text_rect.height() + 5
        
        # 배경 박스
        painter.fillRect(text_x - 2, text_y - text_rect.height() - 2, 
                        text_rect.width() + 4, text_rect.height() + 4, 
                        QColor(0, 0, 0, 180))
        
        # 텍스트 그리기
        painter.drawText(text_x, text_y, max_text)

class MetricGraph(QWidget):
    """작업관리자 스타일 메트릭 그래프 (numpy 없이)"""
    
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
        self.graph_widget = SimpleGraphWidget()
        self.graph_widget.setStyleSheet("""
            QFrame {
                border: 1px solid #333;
                background-color: black;
            }
        """)
        layout.addWidget(self.graph_widget)
    
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
        
        # 그래프 위젯에도 units 전달
        self.graph_widget.units = units
        
        if color:
            self.color = color
            # Convert hex color to QColor
            if color.startswith('#'):
                self.graph_widget.set_color(QColor(color))
        else:
            # Default colors
            colors = ['#00ff00', '#ffaa00', '#ff0000', '#0088ff', '#ff00ff', '#00ffff']
            color = colors[hash(key) % len(colors)]
            self.graph_widget.set_color(QColor(color))
    
    def set_window(self, seconds: int = 60):
        """시간 윈도우 설정"""
        self.window_seconds = seconds
        self.graph_widget.set_window(seconds)
    
    def set_update_rate(self, hz: int = 4):
        """업데이트 주기 설정"""
        self.update_rate_hz = hz
        self.update_timer.start(1000 // hz)
    
    def add_marker(self, timestamp: float, text: str = ""):
        """알림 마커 추가"""
        self.markers.append((timestamp, text))
    
    def clear_markers(self):
        """마커 제거"""
        self.markers.clear()
    
    def _update_graph(self):
        """그래프 업데이트"""
        if not self.metric_bus or not self.metric_key:
            return
        
        # 모니터링이 활성화되지 않았으면 업데이트하지 않음
        if hasattr(self.metric_bus, 'is_running') and not self.metric_bus.is_running:
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
        
        # Update graph
        self.graph_widget.set_data(window_data)
        
        # 디버그 출력
        print(f"그래프 업데이트: {self.metric_key}, 데이터 포인트: {len(window_data)}")
    
    def set_title(self, title: str):
        """제목 설정"""
        self.title = title
        if hasattr(self, 'title_label'):
            self.title_label.setText(title)
    
    def set_color(self, color: str):
        """색상 설정"""
        self.color = color
        if color.startswith('#'):
            self.graph_widget.set_color(QColor(color))
