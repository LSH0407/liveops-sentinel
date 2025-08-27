from PySide6.QtCore import QProcess, QTimer, QByteArray, QObject, Signal
import json
import os
import sys

class BackendProcess(QObject):
    lineReceived = Signal(dict)        # JSON 라인
    readyBanner = Signal(str)          # "BACKEND_READY ..." 원문
    errorText = Signal(str)

    def __init__(self, exe_path: str, parent=None):
        super().__init__(parent)
        self._buf = ""
        self.p = QProcess(self)
        self.p.setProgram(exe_path)
        self.p.setArguments([])        # 절대 cmd.exe 래핑 금지
        self.p.setProcessChannelMode(QProcess.MergedChannels)
        self.p.readyReadStandardOutput.connect(self._on_read)
        self.p.errorOccurred.connect(lambda e: self.errorText.emit(str(e)))
        self.p.finished.connect(lambda *_: self.errorText.emit("backend finished"))

    def start(self):
        self.p.start()                 # startDetached 금지
        
    def isRunning(self): 
        return self.p.state() == QProcess.Running

    def sendJson(self, obj: dict):
        payload = json.dumps(obj) + "\n"
        self.p.write(payload.encode("utf-8"))
        self.p.waitForBytesWritten(1000)

    def _on_read(self):
        chunk = bytes(self.p.readAllStandardOutput()).decode("utf-8", errors="ignore")
        self._buf += chunk
        while True:
            idx = self._buf.find("\n")
            if idx < 0: 
                break
            line = self._buf[:idx].strip()
            self._buf = self._buf[idx+1:]
            if not line: 
                continue
            if line.startswith("BACKEND_READY"):
                self.readyBanner.emit(line)
                continue
            # JSON 라인만 처리
            try:
                obj = json.loads(line)
                self.lineReceived.emit(obj)
            except Exception as e:
                self.errorText.emit(f"parse_error: {e}: {line[:200]}")

    def stop(self):
        try: 
            self.p.kill()
        except: 
            pass
