import subprocess, threading, json, queue, os, sys, time
from pathlib import Path

class BackendProcess:
    def __init__(self, exe_path: str):
        self.exe_path = exe_path
        self.p = None
        self.q = queue.Queue()
        self._alive = False
        self.reader = None

    def start(self):
        if self.p: return
        self.p = subprocess.Popen([self.exe_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        self._alive = True
        self.reader = threading.Thread(target=self._read_loop, daemon=True)
        self.reader.start()

    def stop(self):
        self._alive = False
        if self.p:
            try: 
                self.p.terminate()
                # 3초 대기 후 강제 종료
                try:
                    self.p.wait(timeout=3)
                except:
                    self.p.kill()
            except: pass
        self.p = None

    def send(self, obj: dict):
        if not self.p: return
        line = json.dumps(obj, ensure_ascii=False)
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()

    def _read_loop(self):
        while self._alive and self.p and self.p.stdout:
            line = self.p.stdout.readline()
            if not line: break
            try:
                self.q.put(json.loads(line))
            except: pass

    def poll(self, max_items=100):
        out=[]
        for _ in range(max_items):
            try: out.append(self.q.get_nowait())
            except: break
        return out
