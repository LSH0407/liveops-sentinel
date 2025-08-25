# PyInstaller spec for LiveOps Sentinel GUI
import sys, os
from pathlib import Path
from PyInstaller.utils.hooks import collect_submodules

# robust path (PyInstaller가 __file__ 주입 안 할 때도 동작)
try:
    SPECFILE = Path(__file__).resolve()
except NameError:
    SPECFILE = (Path.cwd() / "packaging" / "liveops_gui.spec").resolve()

GUI_DIR   = SPECFILE.parents[1]     # ui_py
PROJ_ROOT = GUI_DIR.parent          # repo root
MAIN_PY   = GUI_DIR / "main.py"

# backend exe 자동 동봉 (첫 매칭만 사용)
backend_candidates = [
    PROJ_ROOT / "build" / "Release" / "liveops_backend.exe",
    PROJ_ROOT / "build_console" / "Release" / "liveops_backend.exe",
    PROJ_ROOT / "build_gui" / "Release" / "liveops_backend.exe",
]
datas = []
for c in backend_candidates:
    if c.exists():
        datas = [(str(c), ".")]  # dist root에 배치
        break

# 플러그인/숨은 모듈 수집 (Qt, pyqtgraph)
hidden = collect_submodules("PySide6") + collect_submodules("pyqtgraph")

a = Analysis(
    [str(MAIN_PY)],
    pathex=[str(GUI_DIR)],
    binaries=[],
    datas=datas,
    hiddenimports=hidden,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
)
pyz = PYZ(a.pure)
exe = EXE(
    pyz,
    a.scripts,
    exclude_binaries=True,
    name="LiveOps Sentinel",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=False,      # GUI 앱
    icon=None
)
coll = COLLECT(
    exe, a.binaries, a.zipfiles, a.datas,
    name="LiveOpsSentinel"
)
