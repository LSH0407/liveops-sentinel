from PySide6 import QtWidgets, QtCore
from pathlib import Path

class SetupWizard(QtWidgets.QDialog):
    def __init__(self, cfg: dict, parent=None):
        super().__init__(parent)
        self.setWindowTitle("LiveOps Sentinel 첫 설정")
        self.cfg = cfg

        self.webhook = QtWidgets.QLineEdit(cfg.get("webhook",""))
        self.backend = QtWidgets.QLineEdit(cfg.get("backend_path",""))
        browse = QtWidgets.QPushButton("찾기...")
        browse.clicked.connect(self._browse)

        self.rtt  = QtWidgets.QSpinBox(); self.rtt.setRange(1,2000); self.rtt.setValue(cfg["thresholds"]["rttMs"])
        self.loss = QtWidgets.QDoubleSpinBox(); self.loss.setRange(0,100); self.loss.setDecimals(2); self.loss.setValue(cfg["thresholds"]["lossPct"])
        self.hold = QtWidgets.QSpinBox(); self.hold.setRange(1,120); self.hold.setValue(cfg["thresholds"]["holdSec"])

        form = QtWidgets.QFormLayout()
        form.addRow("Discord Webhook", self.webhook)
        h = QtWidgets.QHBoxLayout(); h.addWidget(self.backend); h.addWidget(browse)
        form.addRow("백엔드 실행파일", h)
        th = QtWidgets.QHBoxLayout(); th.addWidget(self.rtt); th.addWidget(self.loss); th.addWidget(self.hold)
        form.addRow("RTT/Loss/Hold", th)

        btn_ok = QtWidgets.QPushButton("저장"); btn_cancel = QtWidgets.QPushButton("취소")
        btn_ok.clicked.connect(self.accept); btn_cancel.clicked.connect(self.reject)
        b = QtWidgets.QHBoxLayout(); b.addStretch(1); b.addWidget(btn_ok); b.addWidget(btn_cancel)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addLayout(form); layout.addStretch(1); layout.addLayout(b); self.resize(520, 220)

    def _browse(self):
        path,_ = QtWidgets.QFileDialog.getOpenFileName(self, "백엔드 실행파일 선택", "", "Executable (*.exe)")
        if path: self.backend.setText(path)

    def result_config(self) -> dict:
        out = dict(self.cfg)
        out["webhook"] = self.webhook.text().strip()
        out["backend_path"] = self.backend.text().strip()
        out["thresholds"] = {"rttMs": self.rtt.value(), "lossPct": self.loss.value(), "holdSec": self.hold.value()}
        return out
