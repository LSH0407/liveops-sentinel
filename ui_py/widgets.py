from PySide6 import QtWidgets, QtCore
import pyqtgraph as pg
import time
from settings import load, save
from setup_wizard import SetupWizard

class MainWindow(QtWidgets.QWidget):
    sendCmd = QtCore.Signal(dict)

    def __init__(self):
        super().__init__()
        self.setWindowTitle("LiveOps Sentinel • GUI")
        
        # 메뉴바 추가
        menubar = QtWidgets.QMenuBar()
        menu = menubar.addMenu("설정")
        action_setup = menu.addAction("설정 마법사")
        action_setup.triggered.connect(self._show_setup)
        
        self.plot_rtt = pg.PlotWidget(title="응답 시간 (ms)")
        self.plot_loss= pg.PlotWidget(title="전송 손실 (%)")
        self.curve_rtt = self.plot_rtt.plot([])
        self.curve_loss= self.plot_loss.plot([])
        self.buf_rtt=[]; self.buf_loss=[]; self.buf_ts=[]; self.maxlen=600

        btn_ping = QtWidgets.QPushButton("Ping")
        btn_get  = QtWidgets.QPushButton("Get Metrics")
        btn_pf   = QtWidgets.QPushButton("Preflight")
        self.webhook = QtWidgets.QLineEdit(); self.webhook.setPlaceholderText("Discord webhook")
        self.rtt  = QtWidgets.QSpinBox(); self.rtt.setRange(1,1000); self.rtt.setValue(80)
        self.loss = QtWidgets.QDoubleSpinBox(); self.loss.setRange(0,100); self.loss.setDecimals(2); self.loss.setValue(2.0)
        self.hold = QtWidgets.QSpinBox(); self.hold.setRange(1,60); self.hold.setValue(5)
        btn_set_th = QtWidgets.QPushButton("Set Thresholds")
        btn_set_wh = QtWidgets.QPushButton("Set Webhook")
        self.log = QtWidgets.QPlainTextEdit(); self.log.setReadOnly(True)

        grid = QtWidgets.QGridLayout(self)
        grid.addWidget(menubar, 0, 0, 1, 2)
        grid.addWidget(self.plot_rtt,1,0,1,2)
        grid.addWidget(self.plot_loss,2,0,1,2)
        grid.addWidget(btn_ping,3,0)
        grid.addWidget(btn_get,3,1)
        grid.addWidget(btn_pf,4,0,1,2)
        grid.addWidget(QtWidgets.QLabel("Webhook"),5,0); grid.addWidget(self.webhook,5,1)
        grid.addWidget(QtWidgets.QLabel("응답시간/손실률/지연시간"),6,0)
        thbox = QtWidgets.QHBoxLayout()
        thbox.addWidget(self.rtt); thbox.addWidget(self.loss); thbox.addWidget(self.hold)
        grid.addLayout(thbox,6,1)
        grid.addWidget(btn_set_th,7,0); grid.addWidget(btn_set_wh,7,1)
        grid.addWidget(self.log,8,0,1,2)

        btn_ping.clicked.connect(lambda: self.sendCmd.emit({"cmd":"ping"}))
        btn_get.clicked.connect(lambda: self.sendCmd.emit({"cmd":"get_metrics"}))
        btn_pf.clicked.connect(lambda: self.sendCmd.emit({"cmd":"preflight"}))
        btn_set_th.clicked.connect(lambda: self.sendCmd.emit({"cmd":"set_thresholds","rttMs":self.rtt.value(),"lossPct":self.loss.value(),"holdSec":self.hold.value()}))
        btn_set_wh.clicked.connect(lambda: self.sendCmd.emit({"cmd":"set_webhook","url":self.webhook.text()}))

        self.timer = QtCore.QTimer(self); self.timer.setInterval(200); self.timer.timeout.connect(self._tick)
        self.timer.start()
        
        # 설정 로드하여 UI에 반영
        self._load_settings()
    
    def _show_setup(self):
        cfg = load()
        wizard = SetupWizard(cfg, self)
        if wizard.exec() == QtWidgets.QDialog.Accepted:
            cfg = wizard.result_config()
            save(cfg)
            self._load_settings()
    
    def _load_settings(self):
        cfg = load()
        self.webhook.setText(cfg.get("webhook", ""))
        self.rtt.setValue(cfg["thresholds"]["rttMs"])
        self.loss.setValue(cfg["thresholds"]["lossPct"])
        self.hold.setValue(cfg["thresholds"]["holdSec"])

    def _tick(self):
        # GUI 쪽에서 주기적으로 재그리기
        if len(self.buf_ts)>0:
            self.curve_rtt.setData(self.buf_rtt[-self.maxlen:])
            self.curve_loss.setData(self.buf_loss[-self.maxlen:])

    def onMessage(self, obj: dict):
        et = obj.get("event")
        if et == "metrics":
            self.buf_ts.append(obj.get("ts", time.time()*1000))
            self.buf_rtt.append(obj.get("rtt_ms",0))
            self.buf_loss.append(obj.get("loss_pct",0))
            if len(self.buf_ts)>self.maxlen:
                self.buf_ts=self.buf_ts[-self.maxlen:]
                self.buf_rtt=self.buf_rtt[-self.maxlen:]
                self.buf_loss=self.buf_loss[-self.maxlen:]
        elif et in ("log","alert","preflight_result","pong"):
            self._append_log(obj)

    def _append_log(self, obj):
        self.log.appendPlainText(str(obj)[:1000])
