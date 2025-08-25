"""
진단 모드 UI 및 리포트 생성
"""

import os
import json
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, List
from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
                               QProgressBar, QPushButton, QTextEdit, QGroupBox)
from PySide6.QtCore import Qt, QTimer, QThread, Signal
from PySide6.QtGui import QFont

class DiagnosticWorker(QThread):
    """진단 작업을 백그라운드에서 실행하는 워커 스레드"""
    
    progress_updated = Signal(int)  # 진행률 (0-100)
    metrics_collected = Signal(dict)  # 수집된 메트릭
    diagnostic_completed = Signal(dict)  # 진단 완료
    
    def __init__(self, duration_seconds: int, platform: str, metric_bus):
        super().__init__()
        self.duration_seconds = duration_seconds
        self.platform = platform
        self.metric_bus = metric_bus
        self.metrics_data = []
        self.running = True
    
    def run(self):
        """진단 실행"""
        start_time = time.time()
        end_time = start_time + self.duration_seconds
        
        while self.running and time.time() < end_time:
            # 진행률 계산
            elapsed = time.time() - start_time
            progress = int((elapsed / self.duration_seconds) * 100)
            self.progress_updated.emit(min(progress, 100))
            
            # 메트릭 수집
            if hasattr(self.metric_bus, 'get_latest_metrics'):
                metrics = self.metric_bus.get_latest_metrics()
                if metrics:
                    self.metrics_data.append({
                        'timestamp': time.time(),
                        'metrics': metrics
                    })
                    self.metrics_collected.emit(metrics)
            
            # 1초 대기
            time.sleep(1)
        
        # 진단 완료
        result = self._generate_diagnostic_report()
        self.diagnostic_completed.emit(result)
    
    def stop(self):
        """진단 중단"""
        self.running = False
    
    def _generate_diagnostic_report(self) -> Dict:
        """진단 리포트 생성"""
        if not self.metrics_data:
            return {"error": "수집된 데이터가 없습니다."}
        
        # 통계 계산
        rtt_values = [m['metrics'].get('net', {}).get('rtt_ms', 0) for m in self.metrics_data]
        loss_values = [m['metrics'].get('net', {}).get('loss_pct', 0) for m in self.metrics_data]
        uplink_values = [m['metrics'].get('net', {}).get('uplink_kbps', 0) for m in self.metrics_data]
        cpu_values = [m['metrics'].get('sys', {}).get('cpu_pct', 0) for m in self.metrics_data]
        memory_values = [m['metrics'].get('sys', {}).get('memory_pct', 0) for m in self.metrics_data]
        
        def calculate_stats(values):
            if not values:
                return {"avg": 0, "max": 0, "min": 0, "p95": 0}
            sorted_values = sorted(values)
            return {
                "avg": sum(values) / len(values),
                "max": max(values),
                "min": min(values),
                "p95": sorted_values[int(len(sorted_values) * 0.95)]
            }
        
        report = {
            "platform": self.platform,
            "duration_seconds": self.duration_seconds,
            "start_time": self.metrics_data[0]['timestamp'] if self.metrics_data else time.time(),
            "end_time": time.time(),
            "total_samples": len(self.metrics_data),
            "statistics": {
                "rtt_ms": calculate_stats(rtt_values),
                "loss_pct": calculate_stats(loss_values),
                "uplink_kbps": calculate_stats(uplink_values),
                "cpu_pct": calculate_stats(cpu_values),
                "memory_pct": calculate_stats(memory_values)
            },
            "raw_data": self.metrics_data
        }
        
        return report

class DiagnosticDialog(QDialog):
    """진단 모드 다이얼로그"""
    
    def __init__(self, duration_seconds: int, platform: str, metric_bus, parent=None):
        super().__init__(parent)
        self.duration_seconds = duration_seconds
        self.platform = platform
        self.metric_bus = metric_bus
        self.worker = None
        self.report_data = None
        
        self.setWindowTitle(f"진단 모드 - {platform}")
        self.setModal(True)
        self.resize(600, 400)
        
        self._setup_ui()
        self._start_diagnostic()
    
    def _setup_ui(self):
        """UI 초기화"""
        layout = QVBoxLayout(self)
        
        # 제목
        title = QLabel(f"진단 모드 실행 중...")
        title.setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;")
        title.setAlignment(Qt.AlignCenter)
        layout.addWidget(title)
        
        # 진행바
        self.progress_bar = QProgressBar()
        self.progress_bar.setStyleSheet("""
            QProgressBar {
                border: 2px solid #404040;
                border-radius: 5px;
                text-align: center;
                background-color: #2d2d2d;
            }
            QProgressBar::chunk {
                background-color: #007bff;
                border-radius: 3px;
            }
        """)
        layout.addWidget(self.progress_bar)
        
        # 상태 정보
        status_group = QGroupBox("진단 상태")
        status_group.setStyleSheet("""
            QGroupBox {
                color: #e0e0e0;
                font-weight: bold;
                border: 1px solid #404040;
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
        
        status_layout = QVBoxLayout(status_group)
        
        self.status_label = QLabel("진단을 시작합니다...")
        self.status_label.setStyleSheet("color: #e0e0e0;")
        status_layout.addWidget(self.status_label)
        
        self.time_label = QLabel("남은 시간: --:--")
        self.time_label.setStyleSheet("color: #b0b0b0;")
        status_layout.addWidget(self.time_label)
        
        layout.addWidget(status_group)
        
        # 실시간 메트릭
        metrics_group = QGroupBox("실시간 메트릭")
        metrics_group.setStyleSheet("""
            QGroupBox {
                color: #e0e0e0;
                font-weight: bold;
                border: 1px solid #404040;
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
        
        metrics_layout = QHBoxLayout(metrics_group)
        
        self.rtt_label = QLabel("응답시간: -- ms")
        self.rtt_label.setStyleSheet("color: #e0e0e0;")
        metrics_layout.addWidget(self.rtt_label)
        
        self.loss_label = QLabel("손실률: -- %")
        self.loss_label.setStyleSheet("color: #e0e0e0;")
        metrics_layout.addWidget(self.loss_label)
        
        self.uplink_label = QLabel("업로드: -- kbps")
        self.uplink_label.setStyleSheet("color: #e0e0e0;")
        metrics_layout.addWidget(self.uplink_label)
        
        layout.addWidget(metrics_group)
        
        # 로그
        log_group = QGroupBox("진단 로그")
        log_group.setStyleSheet("""
            QGroupBox {
                color: #e0e0e0;
                font-weight: bold;
                border: 1px solid #404040;
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
        
        log_layout = QVBoxLayout(log_group)
        
        self.log_text = QTextEdit()
        self.log_text.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e1e;
                color: #e0e0e0;
                border: 1px solid #404040;
                border-radius: 3px;
                font-family: 'Consolas', monospace;
                font-size: 11px;
            }
        """)
        self.log_text.setMaximumHeight(100)
        log_layout.addWidget(self.log_text)
        
        layout.addWidget(log_group)
        
        # 버튼
        button_layout = QHBoxLayout()
        
        self.cancel_button = QPushButton("취소")
        self.cancel_button.setStyleSheet("""
            QPushButton {
                background-color: #dc3545;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #c82333;
            }
        """)
        self.cancel_button.clicked.connect(self._cancel_diagnostic)
        button_layout.addWidget(self.cancel_button)
        
        button_layout.addStretch()
        
        self.export_button = QPushButton("리포트 내보내기")
        self.export_button.setStyleSheet("""
            QPushButton {
                background-color: #28a745;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #218838;
            }
        """)
        self.export_button.clicked.connect(self._export_report)
        self.export_button.setEnabled(False)
        button_layout.addWidget(self.export_button)
        
        layout.addLayout(button_layout)
        
        # 타이머 설정
        self.timer = QTimer()
        self.timer.timeout.connect(self._update_time)
        self.start_time = time.time()
    
    def _start_diagnostic(self):
        """진단 시작"""
        self.worker = DiagnosticWorker(self.duration_seconds, self.platform, self.metric_bus)
        self.worker.progress_updated.connect(self._update_progress)
        self.worker.metrics_collected.connect(self._update_metrics)
        self.worker.diagnostic_completed.connect(self._diagnostic_completed)
        
        self.worker.start()
        self.timer.start(1000)  # 1초마다 업데이트
        
        self.log_text.append(f"[{datetime.now().strftime('%H:%M:%S')}] 진단 시작 - 플랫폼: {self.platform}, 지속시간: {self.duration_seconds}초")
    
    def _update_progress(self, progress: int):
        """진행률 업데이트"""
        self.progress_bar.setValue(progress)
    
    def _update_metrics(self, metrics: Dict):
        """메트릭 업데이트"""
        net_metrics = metrics.get('net', {})
        sys_metrics = metrics.get('sys', {})
        
        rtt = net_metrics.get('rtt_ms', 0)
        loss = net_metrics.get('loss_pct', 0)
        uplink = net_metrics.get('uplink_kbps', 0)
        
        self.rtt_label.setText(f"응답시간: {rtt:.1f} ms")
        self.loss_label.setText(f"손실률: {loss:.2f} %")
        self.uplink_label.setText(f"업로드: {uplink:.0f} kbps")
    
    def _update_time(self):
        """시간 업데이트"""
        elapsed = time.time() - self.start_time
        remaining = max(0, self.duration_seconds - elapsed)
        
        minutes = int(remaining // 60)
        seconds = int(remaining % 60)
        
        self.time_label.setText(f"남은 시간: {minutes:02d}:{seconds:02d}")
        
        if remaining <= 0:
            self.timer.stop()
    
    def _diagnostic_completed(self, report: Dict):
        """진단 완료"""
        self.report_data = report
        self.timer.stop()
        
        self.status_label.setText("진단 완료!")
        self.time_label.setText("완료됨")
        self.cancel_button.setText("닫기")
        self.export_button.setEnabled(True)
        
        # 통계 요약 표시
        stats = report.get('statistics', {})
        summary = f"""
진단 완료!

📊 통계 요약:
• 응답시간: 평균 {stats.get('rtt_ms', {}).get('avg', 0):.1f}ms (최대 {stats.get('rtt_ms', {}).get('max', 0):.1f}ms)
• 손실률: 평균 {stats.get('loss_pct', {}).get('avg', 0):.2f}% (최대 {stats.get('loss_pct', {}).get('max', 0):.2f}%)
• 업로드: 평균 {stats.get('uplink_kbps', {}).get('avg', 0):.0f}kbps (최대 {stats.get('uplink_kbps', {}).get('max', 0):.0f}kbps)
• CPU: 평균 {stats.get('cpu_pct', {}).get('avg', 0):.1f}% (최대 {stats.get('cpu_pct', {}).get('max', 0):.1f}%)
• 메모리: 평균 {stats.get('memory_pct', {}).get('avg', 0):.1f}% (최대 {stats.get('memory_pct', {}).get('max', 0):.1f}%)

총 {report.get('total_samples', 0)}개 샘플 수집
        """
        
        self.log_text.append(summary.strip())
    
    def _cancel_diagnostic(self):
        """진단 취소"""
        if self.worker and self.worker.isRunning():
            self.worker.stop()
            self.worker.wait()
        
        self.accept()
    
    def _export_report(self):
        """리포트 내보내기"""
        if not self.report_data:
            return
        
        # 리포트 저장 경로
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        session_id = f"session_{timestamp}"
        
        # Documents 폴더에 저장
        documents_path = Path.home() / "Documents" / "LiveOpsReports"
        session_path = documents_path / session_id
        session_path.mkdir(parents=True, exist_ok=True)
        
        # JSON 리포트 저장
        report_file = session_path / "diagnostic_report.json"
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(self.report_data, f, indent=2, ensure_ascii=False)
        
        # CSV 메트릭 데이터 저장
        csv_file = session_path / "metrics.csv"
        with open(csv_file, 'w', encoding='utf-8') as f:
            f.write("timestamp,rtt_ms,loss_pct,uplink_kbps,cpu_pct,memory_pct\n")
            for data in self.report_data.get('raw_data', []):
                metrics = data['metrics']
                net = metrics.get('net', {})
                sys = metrics.get('sys', {})
                f.write(f"{data['timestamp']},{net.get('rtt_ms', 0)},{net.get('loss_pct', 0)},{net.get('uplink_kbps', 0)},{sys.get('cpu_pct', 0)},{sys.get('memory_pct', 0)}\n")
        
        # HTML 리포트 생성
        html_file = session_path / "report.html"
        self._generate_html_report(html_file)
        
        # ZIP 파일 생성
        try:
            from core.zip_exporter import ZipExporter
            exporter = ZipExporter()
            zip_path = exporter.create_report_zip(session_path, self.platform, self.duration_seconds)
            
            self.log_text.append(f"\n📁 리포트 저장 완료: {session_path}")
            self.log_text.append(f"• JSON: {report_file.name}")
            self.log_text.append(f"• CSV: {csv_file.name}")
            self.log_text.append(f"• HTML: {html_file.name}")
            self.log_text.append(f"• ZIP: {zip_path.name}")
            
        except Exception as e:
            self.log_text.append(f"\n📁 리포트 저장 완료: {session_path}")
            self.log_text.append(f"• JSON: {report_file.name}")
            self.log_text.append(f"• CSV: {csv_file.name}")
            self.log_text.append(f"• HTML: {html_file.name}")
            self.log_text.append(f"• ZIP 생성 실패: {e}")
    
    def _generate_html_report(self, html_file: Path):
        """HTML 리포트 생성"""
        stats = self.report_data.get('statistics', {})
        
        html_content = f"""
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LiveOps Sentinel 진단 리포트</title>
    <style>
        body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background-color: #1e1e1e; color: #e0e0e0; }}
        .container {{ max-width: 1200px; margin: 0 auto; }}
        .header {{ text-align: center; margin-bottom: 30px; }}
        .header h1 {{ color: #007bff; margin-bottom: 10px; }}
        .summary {{ background-color: #2d2d2d; padding: 20px; border-radius: 8px; margin-bottom: 20px; }}
        .metrics-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin-bottom: 20px; }}
        .metric-card {{ background-color: #2d2d2d; padding: 15px; border-radius: 8px; border-left: 4px solid #007bff; }}
        .metric-card h3 {{ margin: 0 0 10px 0; color: #007bff; }}
        .metric-value {{ font-size: 24px; font-weight: bold; margin-bottom: 5px; }}
        .metric-label {{ font-size: 12px; color: #888; }}
        .recommendations {{ background-color: #2d2d2d; padding: 20px; border-radius: 8px; }}
        .recommendations h2 {{ color: #28a745; margin-top: 0; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>LiveOps Sentinel 진단 리포트</h1>
            <p>플랫폼: {self.report_data.get('platform', 'Unknown')} | 지속시간: {self.report_data.get('duration_seconds', 0)}초</p>
            <p>생성 시간: {datetime.fromtimestamp(self.report_data.get('start_time', time.time())).strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
        
        <div class="summary">
            <h2>진단 요약</h2>
            <p>총 {self.report_data.get('total_samples', 0)}개 샘플을 수집하여 네트워크 및 시스템 상태를 분석했습니다.</p>
        </div>
        
        <div class="metrics-grid">
            <div class="metric-card">
                <h3>응답 시간</h3>
                <div class="metric-value">{stats.get('rtt_ms', {}).get('avg', 0):.1f}ms</div>
                <div class="metric-label">평균 (최대: {stats.get('rtt_ms', {}).get('max', 0):.1f}ms)</div>
            </div>
            <div class="metric-card">
                <h3>패킷 손실</h3>
                <div class="metric-value">{stats.get('loss_pct', {}).get('avg', 0):.2f}%</div>
                <div class="metric-label">평균 (최대: {stats.get('loss_pct', {}).get('max', 0):.2f}%)</div>
            </div>
            <div class="metric-card">
                <h3>업로드 대역폭</h3>
                <div class="metric-value">{stats.get('uplink_kbps', {}).get('avg', 0):.0f}kbps</div>
                <div class="metric-label">평균 (최대: {stats.get('uplink_kbps', {}).get('max', 0):.0f}kbps)</div>
            </div>
            <div class="metric-card">
                <h3>CPU 사용률</h3>
                <div class="metric-value">{stats.get('cpu_pct', {}).get('avg', 0):.1f}%</div>
                <div class="metric-label">평균 (최대: {stats.get('cpu_pct', {}).get('max', 0):.1f}%)</div>
            </div>
            <div class="metric-card">
                <h3>메모리 사용률</h3>
                <div class="metric-value">{stats.get('memory_pct', {}).get('avg', 0):.1f}%</div>
                <div class="metric-label">평균 (최대: {stats.get('memory_pct', {}).get('max', 0):.1f}%)</div>
            </div>
        </div>
        
        <div class="recommendations">
            <h2>권장사항</h2>
            <ul>
                <li>응답 시간이 100ms를 초과하는 경우 유선 연결을 사용하거나 공유기 QoS 설정을 확인하세요.</li>
                <li>패킷 손실이 2%를 초과하는 경우 비트레이트를 20% 낮추거나 네트워크 연결을 점검하세요.</li>
                <li>업로드 대역폭의 70% 이하로 비트레이트를 설정하여 안정성을 확보하세요.</li>
                <li>CPU 사용률이 80%를 초과하는 경우 해상도를 낮추거나 HW 인코더를 사용하세요.</li>
            </ul>
        </div>
    </div>
</body>
</html>
        """
        
        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
