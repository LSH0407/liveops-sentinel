# -*- mode: python ; coding: utf-8 -*-

import os
import sys
from pathlib import Path

# 숨겨진 import들
hiddenimports = [
    'PySide6.QtCore',
    'PySide6.QtWidgets',
    'PySide6.QtGui',
    'obsws_python',
    'json',
    'pathlib',
    'datetime',
    'zipfile',
    'pyqtgraph',
    'numpy',
    'numpy._core',
    'numpy._core._exceptions',
    'numpy._core.multiarray',
    'numpy._core.umath',
    'numpy._core.numeric',
    'numpy._core.fromnumeric',
    'numpy._core.defchararray',
    'numpy._core.records',
    'numpy._core.memmap',
    'numpy._core.function_base',
    'numpy._core.machar',
    'numpy._core.getlimits',
    'numpy._core.shape_base',
    'numpy._core._dtype_ctypes',
    'numpy._core._methods',
    'numpy._core._dtype',
    'numpy._core._internal',
    'numpy._core._ufunc_config',
    'numpy._core._add_newdocs',
    'numpy._core._add_newdocs_scalars',
    'numpy._core._dtype_ctypes',
    'numpy._core._methods',
    'numpy._core._dtype',
    'numpy._core._internal',
    'numpy._core._ufunc_config',
    'numpy._core._add_newdocs',
    'numpy._core._add_newdocs_scalars',
    'Jinja2',
    'psutil',
]

# 분석 제외
excludes = [
    'matplotlib',
    'scipy',
    'pandas',
    'tkinter',
    'wx',
    'IPython',
    'jupyter',
]

# 데이터 파일들
datas = [
    ('../settings.py', '.'),
    ('../setup_wizard.py', '.'),
    ('../platform_rules.py', '.'),
]

# 백엔드 실행 파일 (존재하는 경우에만 추가)
backend_path = Path('../../build/Release/liveops_backend.exe')
if backend_path.exists():
    datas.append((str(backend_path), '.'))

a = Analysis(
    ['../main.py'],
    pathex=['..'],
    binaries=[],
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=excludes,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=None,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=None)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='LiveOpsSentinel',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,  # GUI 모드
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon='../../assets/icon.ico' if Path('../../assets/icon.ico').exists() else None,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='LiveOpsSentinel',
)
