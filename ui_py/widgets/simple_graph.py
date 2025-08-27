from PySide6.QtCore import QTimer, Qt, QPointF
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QFrame
from PySide6.QtGui import QPainter, QPen, QBrush, QColor, QPolygonF, QFont
import time
from typing import List, Tuple, Optional

class SimpleGraphWidget(QFrame):
    """간단한 Qt 기반 실시간 그래프"""
    
    def __init__(self, title: str = "", parent=None):
        super().__init__(parent)
        self.title = title
        self.data_points = []  # [(timestamp, value), ...]
        self.window_seconds = 60
        self.color = self._get_color_by_title(title)  # 제목에 따른 색상 자동 설정
        self.fill_under = True
        self.is_active = False
        
        self.setMinimumHeight(150)
        self.setMinimumWidth(300)
        self.setMaximumHeight(200)
        self.setMaximumWidth(400)
        
        # 스타일 설정
        self.setStyleSheet("""
            QFrame {
                border: 2px solid #444;
                background-color: black;
                border-radius: 5px;
            }
        """)
        
        # 타이머 설정
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self._update_display)
        self.update_timer.start(250)  # 4Hz
    
    def set_active(self, active: bool):
        """그래프 활성화/비활성화"""
        self.is_active = active
        if not active:
            self.update_timer.stop()
        else:
            self.update_timer.start(250)
    
    def add_point(self, timestamp: float, value: float):
        """새 데이터 포인트 추가"""
        print(f"SimpleGraphWidget.add_point: timestamp={timestamp}, value={value}")
        self.data_points.append((timestamp, value))
        
        # 링 버퍼 크기 유지 (240개 포인트 = 60초)
        if len(self.data_points) > 240:
            self.data_points.pop(0)
        
        print(f"SimpleGraphWidget 데이터 추가됨. 현재 개수: {len(self.data_points)}")
    
    def _update_display(self):
        """화면 업데이트"""
        if not self.is_active:
            return
        self.update()
    
    def paintEvent(self, event):
        """그래프 그리기"""
        try:
            painter = QPainter(self)
            painter.setRenderHint(QPainter.Antialiasing)
            
            # 배경
            painter.fillRect(self.rect(), QColor(0, 0, 0))
            
            # 제목
            if self.title:
                painter.setPen(QPen(QColor(224, 224, 224)))
                painter.setFont(QFont("Arial", 10, QFont.Bold))
                painter.drawText(10, 20, self.title)
            
            # 데이터 필터링 (시간 윈도우)
            now = time.time()
            window_data = [(ts, val) for ts, val in self.data_points if now - ts <= self.window_seconds]
            
            if not window_data:
                # 데이터가 없으면 메시지 표시
                painter.setPen(QPen(QColor(128, 128, 128)))
                painter.setFont(QFont("Arial", 9))
                painter.drawText(self.rect(), Qt.AlignCenter, "데이터 없음")
                return
            
            # 좌표 변환
            width = self.width() - 40  # 여백 제외
            height = self.height() - 60  # 제목과 여백 제외
            
            if width <= 0 or height <= 0:
                return
            
            # 데이터 정규화
            values = [val for _, val in window_data]
            if not values:
                return
            
            min_val = min(values)
            max_val = max(values)
            if min_val == max_val:
                min_val -= 1
                max_val += 1
            
            # 그리드 그리기 (더 부드럽게)
            painter.setPen(QPen(QColor(32, 32, 32), 1))
            for i in range(0, width, width // 6):
                painter.drawLine(20 + i, 40, 20 + i, 40 + height)
            for i in range(0, height, height // 4):
                painter.drawLine(20, 40 + i, 20 + width, 40 + i)
            
            # 메인 그리드 라인 (더 진하게)
            painter.setPen(QPen(QColor(48, 48, 48), 1))
            painter.drawLine(20, 40, 20, 40 + height)  # Y축
            painter.drawLine(20, 40 + height, 20 + width, 40 + height)  # X축
            
            # 데이터 포인트 그리기
            if len(window_data) > 1:
                # 선 그리기 (그라데이션 효과)
                points = []
                for i, (ts, val) in enumerate(window_data):
                    x = 20 + (i / (len(window_data) - 1)) * width
                    y = 40 + height - ((val - min_val) / (max_val - min_val)) * height
                    points.append(QPointF(x, y))
                
                # 부드러운 곡선 그리기
                if len(points) >= 3:
                    # 베지어 곡선을 위한 제어점 계산
                    for i in range(len(points) - 2):
                        # 색상 그라데이션 (오래된 데이터는 어둡게)
                        alpha = int(255 * (i / len(points)))
                        color = QColor(self.color)
                        color.setAlpha(alpha)
                        painter.setPen(QPen(color, 2))
                        
                        # 부드러운 곡선 그리기
                        path = QPolygonF()
                        path.append(points[i])
                        path.append(points[i + 1])
                        path.append(points[i + 2])
                        painter.drawPolyline(path)
                else:
                    # 데이터가 적을 때는 직선
                    for i in range(len(points) - 1):
                        alpha = int(255 * (i / len(points)))
                        color = QColor(self.color)
                        color.setAlpha(alpha)
                        painter.setPen(QPen(color, 2))
                        painter.drawLine(points[i], points[i + 1])
                
                # 최근 선은 밝게
                painter.setPen(QPen(self.color, 3))
                if len(points) >= 2:
                    painter.drawLine(points[-2], points[-1])
                
                # 포인트 제거 - 깔끔한 선만 표시
            
            # 축 레이블
            painter.setPen(QPen(QColor(224, 224, 224)))
            painter.setFont(QFont("Arial", 8))
            painter.drawText(10, self.height() - 10, "Time (s)")
            painter.drawText(5, 30, f"{max_val:.1f}")
            painter.drawText(5, 40 + height // 2, f"{(min_val + max_val) / 2:.1f}")
            painter.drawText(5, 40 + height, f"{min_val:.1f}")
            
            # 현재 값 표시 (더 깔끔하게)
            if window_data:
                current_val = window_data[-1][1]
                # 배경 박스
                painter.setBrush(QBrush(QColor(0, 0, 0, 180)))
                painter.setPen(QPen(QColor(0, 0, 0, 0)))
                painter.drawRect(self.width() - 90, 15, 85, 20)
                
                # 텍스트
                painter.setPen(QPen(self.color))
                painter.setFont(QFont("Arial", 9, QFont.Bold))
                painter.drawText(self.width() - 85, 28, f"{current_val:.1f}")
            
        except Exception as e:
            print(f"SimpleGraphWidget paintEvent 오류: {e}")
            import traceback
            traceback.print_exc()
    
    def clear(self):
        """그래프 초기화"""
        self.data_points.clear()
        self.update()
    
    def _get_color_by_title(self, title: str) -> QColor:
        """제목에 따른 색상 반환"""
        title_lower = title.lower()
        
        if 'rtt' in title_lower or '응답' in title_lower:
            return QColor(0, 255, 255)  # 시안 (서버 응답 속도)
        elif 'loss' in title_lower or '손실' in title_lower:
            return QColor(255, 165, 0)  # 주황 (전송 손실)
        elif 'cpu' in title_lower:
            return QColor(255, 0, 255)  # 마젠타 (CPU)
        elif 'gpu' in title_lower:
            return QColor(255, 255, 0)  # 노랑 (GPU)
        elif 'dropped' in title_lower or '버린' in title_lower:
            return QColor(255, 0, 0)    # 빨강 (버린 프레임)
        elif 'encoding' in title_lower or '인코딩' in title_lower:
            return QColor(0, 255, 0)    # 녹색 (인코딩 지연)
        elif 'render' in title_lower or '렌더' in title_lower:
            return QColor(0, 0, 255)    # 파랑 (렌더 지연)
        else:
            return QColor(0, 255, 0)    # 기본 녹색
