from PySide6 import QtWidgets, QtCore
from pathlib import Path

class SetupWizard(QtWidgets.QDialog):
    def __init__(self, cfg: dict, parent=None):
        super().__init__(parent)
        self.setWindowTitle("LiveOps Sentinel 첫 설정")
        self.cfg = cfg

        # 백엔드 실행 파일 자동 찾기
        backend_path = self._find_backend_path()
        
        self.webhook = QtWidgets.QLineEdit(cfg.get("webhook",""))
        self.backend = QtWidgets.QLineEdit(backend_path)
        browse = QtWidgets.QPushButton("찾기...")
        browse.clicked.connect(self._browse)

        # 사용자 친화적인 용어로 변경
        self.response_time = QtWidgets.QSpinBox()
        self.response_time.setRange(1, 2000)
        self.response_time.setValue(cfg.get("thresholds", {}).get("rttMs", 100))
        self.response_time.setSuffix(" ms")
        self.response_time.setToolTip("네트워크 응답 시간 (낮을수록 좋음)")
        
        self.packet_loss = QtWidgets.QDoubleSpinBox()
        self.packet_loss.setRange(0, 100)
        self.packet_loss.setDecimals(2)
        self.packet_loss.setValue(cfg.get("thresholds", {}).get("lossPct", 2.0))
        self.packet_loss.setSuffix(" %")
        self.packet_loss.setToolTip("패킷 손실률 (0%에 가까울수록 좋음)")
        
        self.alert_delay = QtWidgets.QSpinBox()
        self.alert_delay.setRange(1, 120)
        self.alert_delay.setValue(cfg.get("thresholds", {}).get("holdSec", 10))
        self.alert_delay.setSuffix(" 초")
        self.alert_delay.setToolTip("경고 지속 시간 (문제가 지속될 때 알림)")

        form = QtWidgets.QFormLayout()
        form.addRow("Discord Webhook (선택사항)", self.webhook)
        
        # 백엔드 실행 파일 섹션
        backend_label = QtWidgets.QLabel("백엔드 실행 파일")
        backend_label.setToolTip("LiveOps Sentinel 백엔드 프로그램 위치")
        h = QtWidgets.QHBoxLayout()
        h.addWidget(self.backend)
        h.addWidget(browse)
        form.addRow(backend_label, h)
        
        # 임계값 섹션
        threshold_label = QtWidgets.QLabel("경고 임계값")
        threshold_label.setToolTip("이 값들을 초과하면 경고가 발생합니다")
        th = QtWidgets.QHBoxLayout()
        th.addWidget(QtWidgets.QLabel("응답시간:"))
        th.addWidget(self.response_time)
        th.addWidget(QtWidgets.QLabel("손실률:"))
        th.addWidget(self.packet_loss)
        th.addWidget(QtWidgets.QLabel("지연시간:"))
        th.addWidget(self.alert_delay)
        form.addRow(threshold_label, th)

        # 설명 텍스트 추가
        info_text = QtWidgets.QLabel(
            "💡 응답시간: 네트워크 지연 (낮을수록 좋음)\n"
            "💡 손실률: 데이터 전송 실패율 (0%에 가까울수록 좋음)\n"
            "💡 지연시간: 문제 지속 시 알림까지 대기 시간"
        )
        info_text.setStyleSheet("color: #888; font-size: 11px;")
        form.addRow("", info_text)

        btn_ok = QtWidgets.QPushButton("저장")
        btn_cancel = QtWidgets.QPushButton("취소")
        btn_ok.clicked.connect(self.accept)
        btn_cancel.clicked.connect(self.reject)
        b = QtWidgets.QHBoxLayout()
        b.addStretch(1)
        b.addWidget(btn_ok)
        b.addWidget(btn_cancel)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addLayout(form)
        layout.addStretch(1)
        layout.addLayout(b)
        self.resize(600, 300)

    def _find_backend_path(self) -> str:
        """백엔드 실행 파일을 자동으로 찾습니다"""
        candidates = [
            Path("../build/Release/liveops_backend.exe"),
            Path("../build_console/Release/liveops_backend.exe"),
            Path("../build_gui/Release/liveops_backend.exe"),
            Path("../../build/Release/liveops_backend.exe"),
        ]
        
        for candidate in candidates:
            full_path = (Path(__file__).parent / candidate).resolve()
            if full_path.exists():
                return str(full_path)
        
        # 기본값 반환
        return str((Path(__file__).parent / "../build/Release/liveops_backend.exe").resolve())

    def _browse(self):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self, 
            "백엔드 실행파일 선택", 
            str(Path(self.backend.text()).parent) if self.backend.text() else "",
            "Executable (*.exe)"
        )
        if path:
            self.backend.setText(path)

    def result_config(self) -> dict:
        out = dict(self.cfg)
        out["webhook"] = self.webhook.text().strip()
        out["backend_path"] = self.backend.text().strip()
        out["thresholds"] = {
            "rttMs": self.response_time.value(),
            "lossPct": self.packet_loss.value(),
            "holdSec": self.alert_delay.value()
        }
        return out
