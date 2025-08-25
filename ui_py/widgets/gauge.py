from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QPainter, QPen, QBrush, QColor, QFont
import math

class GaugeWidget(QWidget):
    """도넛형 게이지 위젯"""
    
    def __init__(self, title: str = "", parent=None):
        super().__init__(parent)
        self.title = title
        self.score = 0
        self.grade = "좋음"
        self.recommendation = ""
        
        self._setup_ui()
        self._apply_dark_theme()
        
        # Animation timer
        self.animation_timer = QTimer()
        self.animation_timer.timeout.connect(self._update_animation)
        self.target_score = 0
        self.animation_speed = 5
        
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(8)
        
        # Title
        self.title_label = QLabel(self.title)
        self.title_label.setAlignment(Qt.AlignCenter)
        self.title_label.setStyleSheet("""
            QLabel {
                color: #e0e0e0;
                font-size: 14px;
                font-weight: bold;
            }
        """)
        layout.addWidget(self.title_label)
        
        # Score display
        self.score_label = QLabel("0")
        self.score_label.setAlignment(Qt.AlignCenter)
        self.score_label.setStyleSheet("""
            QLabel {
                color: #ffffff;
                font-size: 32px;
                font-weight: bold;
            }
        """)
        layout.addWidget(self.score_label)
        
        # Grade display
        self.grade_label = QLabel("좋음")
        self.grade_label.setAlignment(Qt.AlignCenter)
        self.grade_label.setStyleSheet("""
            QLabel {
                color: #28a745;
                font-size: 16px;
                font-weight: bold;
            }
        """)
        layout.addWidget(self.grade_label)
        
        # Recommendation
        self.recommendation_label = QLabel("")
        self.recommendation_label.setAlignment(Qt.AlignCenter)
        self.recommendation_label.setWordWrap(True)
        self.recommendation_label.setStyleSheet("""
            QLabel {
                color: #b0b0b0;
                font-size: 11px;
                font-weight: normal;
            }
        """)
        layout.addWidget(self.recommendation_label)
        
        # Set fixed size
        self.setFixedSize(200, 250)
    
    def _apply_dark_theme(self):
        """다크 테마 적용"""
        self.setStyleSheet("""
            GaugeWidget {
                background-color: #2d2d2d;
                border: 1px solid #404040;
                border-radius: 10px;
            }
        """)
    
    def set_score(self, score: float, animate: bool = True):
        """점수 설정"""
        if animate:
            self.target_score = score
            self.animation_timer.start(50)  # 20 FPS
        else:
            self.score = score
            self._update_display()
    
    def set_grade(self, grade: str):
        """등급 설정"""
        self.grade = grade
        self.grade_label.setText(grade)
        
        # Update color based on grade
        if grade == "좋음":
            color = "#28a745"  # Green
        elif grade == "주의":
            color = "#ffc107"  # Yellow
        else:  # 불안정
            color = "#dc3545"  # Red
        
        self.grade_label.setStyleSheet(f"""
            QLabel {{
                color: {color};
                font-size: 16px;
                font-weight: bold;
            }}
        """)
    
    def set_recommendation(self, text: str):
        """권장사항 설정"""
        self.recommendation = text
        self.recommendation_label.setText(text)
    
    def _update_animation(self):
        """애니메이션 업데이트"""
        if abs(self.score - self.target_score) < 1:
            self.score = self.target_score
            self.animation_timer.stop()
        else:
            diff = self.target_score - self.score
            self.score += diff * 0.1  # Smooth interpolation
        
        self._update_display()
    
    def _update_display(self):
        """디스플레이 업데이트"""
        self.score_label.setText(f"{int(self.score)}")
        self.update()  # Trigger paint event
    
    def paintEvent(self, event):
        """그리기 이벤트"""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        # Calculate gauge dimensions
        center_x = self.width() // 2
        center_y = self.height() // 2 - 20  # Offset for text
        outer_radius = min(center_x, center_y) - 20
        inner_radius = outer_radius * 0.6
        thickness = outer_radius - inner_radius
        
        # Draw background circle
        painter.setPen(QPen(QColor("#404040"), thickness))
        painter.drawEllipse(center_x - outer_radius, center_y - outer_radius, 
                           outer_radius * 2, outer_radius * 2)
        
        # Calculate progress angle
        progress = self.score / 100.0
        angle = progress * 360
        
        # Draw progress arc
        if angle > 0:
            # Determine color based on score
            if self.score >= 85:
                color = QColor("#28a745")  # Green
            elif self.score >= 60:
                color = QColor("#ffc107")  # Yellow
            else:
                color = QColor("#dc3545")  # Red
            
            painter.setPen(QPen(color, thickness))
            painter.drawArc(center_x - outer_radius, center_y - outer_radius,
                           outer_radius * 2, outer_radius * 2,
                           -90 * 16, -angle * 16)  # Start from top, clockwise
    
    def get_score(self) -> float:
        """현재 점수 반환"""
        return self.score
    
    def get_grade(self) -> str:
        """현재 등급 반환"""
        return self.grade
