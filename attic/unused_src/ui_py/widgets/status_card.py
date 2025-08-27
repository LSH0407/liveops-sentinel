from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont

class StatusCard(QWidget):
    """상태 카드 위젯 (큰 수치 + 서브 텍스트 + grade pill)"""
    
    def __init__(self, title: str = "", units: str = "", parent=None):
        super().__init__(parent)
        self.title = title
        self.units = units
        self.current_value = 0
        self.current_grade = "좋음"
        
        self._setup_ui()
        self._apply_dark_theme()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 8, 12, 8)
        layout.setSpacing(4)
        
        # Title
        self.title_label = QLabel(self.title)
        self.title_label.setAlignment(Qt.AlignCenter)
        self.title_label.setStyleSheet("""
            QLabel {
                color: #b0b0b0;
                font-size: 11px;
                font-weight: normal;
            }
        """)
        layout.addWidget(self.title_label)
        
        # Value and units
        value_layout = QHBoxLayout()
        value_layout.setSpacing(4)
        
        self.value_label = QLabel("0")
        self.value_label.setAlignment(Qt.AlignCenter)
        self.value_label.setStyleSheet("""
            QLabel {
                color: #ffffff;
                font-size: 24px;
                font-weight: bold;
            }
        """)
        
        self.units_label = QLabel(self.units)
        self.units_label.setAlignment(Qt.AlignLeft | Qt.AlignBottom)
        self.units_label.setStyleSheet("""
            QLabel {
                color: #b0b0b0;
                font-size: 12px;
                font-weight: normal;
            }
        """)
        
        value_layout.addWidget(self.value_label)
        value_layout.addWidget(self.units_label)
        value_layout.addStretch()
        layout.addLayout(value_layout)
        
        # Grade pill
        self.grade_label = QLabel("좋음")
        self.grade_label.setAlignment(Qt.AlignCenter)
        self.grade_label.setStyleSheet("""
            QLabel {
                color: #ffffff;
                font-size: 10px;
                font-weight: bold;
                background-color: #28a745;
                border-radius: 8px;
                padding: 2px 8px;
            }
        """)
        layout.addWidget(self.grade_label)
        
        # Hint text
        self.hint_label = QLabel("")
        self.hint_label.setAlignment(Qt.AlignCenter)
        self.hint_label.setWordWrap(True)
        self.hint_label.setStyleSheet("""
            QLabel {
                color: #888888;
                font-size: 9px;
                font-weight: normal;
            }
        """)
        layout.addWidget(self.hint_label)
        
        # Set fixed height for consistent layout
        self.setFixedHeight(120)
    
    def _apply_dark_theme(self):
        """다크 테마 적용"""
        self.setStyleSheet("""
            StatusCard {
                background-color: #2d2d2d;
                border: 1px solid #404040;
                border-radius: 8px;
            }
        """)
    
    def set_value(self, value: float, units: str = None):
        """값 설정"""
        self.current_value = value
        
        # Format value based on magnitude
        if value >= 1000:
            formatted_value = f"{value/1000:.1f}k"
        elif value >= 100:
            formatted_value = f"{value:.0f}"
        elif value >= 10:
            formatted_value = f"{value:.1f}"
        else:
            formatted_value = f"{value:.2f}"
        
        self.value_label.setText(formatted_value)
        
        if units:
            self.units_label.setText(units)
    
    def set_grade(self, grade: str):
        """등급 설정"""
        self.current_grade = grade
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
                color: #ffffff;
                font-size: 10px;
                font-weight: bold;
                background-color: {color};
                border-radius: 8px;
                padding: 2px 8px;
            }}
        """)
    
    def set_hint(self, text: str):
        """힌트 텍스트 설정"""
        self.hint_label.setText(text)
    
    def set_title(self, title: str):
        """제목 설정"""
        self.title = title
        self.title_label.setText(title)
    
    def get_value(self) -> float:
        """현재 값 반환"""
        return self.current_value
    
    def get_grade(self) -> str:
        """현재 등급 반환"""
        return self.current_grade
