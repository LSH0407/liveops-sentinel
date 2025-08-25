from PySide6 import QtWidgets, QtCore
from pathlib import Path
import sys, os, json, time
from core.metric_bus import MetricBus
from views.dashboard import DashboardView
from settings import load, save, setup_logging
from setup_wizard import SetupWizard

def default_backend_path():
    # 빌드 산출물 추정 (필요시 설정창에서 바꿀 수 있음)
    candidates = [
        Path("../build_backend/Release/liveops_backend.exe"),
        Path("../build/Release/liveops_backend.exe"),
        Path("../build_console/Release/liveops_backend.exe"),
        Path("../build_gui/Release/liveops_backend.exe"),
    ]
    for c in candidates:
        p = (Path(__file__).parent / c).resolve()
        if p.exists(): return str(p)
    return str((Path(__file__).parent / "../build_backend/Release/liveops_backend.exe").resolve())

def setup_high_dpi():
    """고DPI 설정"""
    # Qt6에서는 자동으로 고DPI를 지원하므로 별도 설정 불필요
    pass

def setup_dark_theme(app):
    """다크 테마 설정"""
    palette = app.palette()
    palette.setColor(palette.ColorRole.Window, QtCore.Qt.black)
    palette.setColor(palette.ColorRole.WindowText, QtCore.Qt.white)
    palette.setColor(palette.ColorRole.Base, QtCore.Qt.darkGray)
    palette.setColor(palette.ColorRole.AlternateBase, QtCore.Qt.lightGray)
    palette.setColor(palette.ColorRole.ToolTipBase, QtCore.Qt.white)
    palette.setColor(palette.ColorRole.ToolTipText, QtCore.Qt.white)
    palette.setColor(palette.ColorRole.Text, QtCore.Qt.white)
    palette.setColor(palette.ColorRole.Button, QtCore.Qt.darkGray)
    palette.setColor(palette.ColorRole.ButtonText, QtCore.Qt.white)
    palette.setColor(palette.ColorRole.BrightText, QtCore.Qt.red)
    palette.setColor(palette.ColorRole.Link, QtCore.Qt.cyan)
    palette.setColor(palette.ColorRole.Highlight, QtCore.Qt.darkCyan)
    palette.setColor(palette.ColorRole.HighlightedText, QtCore.Qt.black)
    app.setPalette(palette)



def main():
    try:
        app = QtWidgets.QApplication(sys.argv)
        
        # 고DPI 및 다크 테마 설정
        setup_high_dpi()
        setup_dark_theme(app)
        
        # 로깅 시스템 초기화
        setup_logging()
        
        # 설정 로드
        cfg = load()
        
        # 백엔드 경로 결정 (설정 우선, 없으면 기본값)
        backend_path = cfg.get("backend_path", "")
        if not backend_path or not Path(backend_path).exists():
            backend_path = default_backend_path()
        
        # MetricBus 초기화
        metric_bus = MetricBus(backend_path)
        metric_bus.start()
        
        # 대시보드 뷰 생성
        dashboard = DashboardView(metric_bus, cfg)
        dashboard.set_current_bitrate(cfg.get("current_bitrate_kbps", 6000))
        
        # 윈도우 설정
        dashboard.setWindowTitle("LiveOps Sentinel • 모니터링")
        dashboard.resize(1400, 900)
        
        # 윈도우 위치 조정 (화면 중앙에 위치)
        screen = app.primaryScreen()
        screen_geometry = screen.geometry()
        window_geometry = dashboard.geometry()
        
        x = (screen_geometry.width() - window_geometry.width()) // 2
        y = (screen_geometry.height() - window_geometry.height()) // 2
        y = max(50, y)  # 최소 50px 위쪽 여백 확보
        
        dashboard.move(x, y)
        dashboard.show()

        ret = app.exec()
        
        # 안전한 종료
        try:
            metric_bus.stop()
        except:
            pass
        
        # 모든 프로세스 강제 종료
        try:
            import subprocess
            subprocess.run(['taskkill', '/f', '/im', 'liveops_backend.exe'], 
                         capture_output=True, timeout=5)
        except:
            pass
        
        sys.exit(ret)
        
    except Exception as e:
        print(f"애플리케이션 시작 오류: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
