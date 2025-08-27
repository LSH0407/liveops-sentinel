from __future__ import annotations
import json, os, logging
from pathlib import Path
from logging.handlers import RotatingFileHandler

APP_DIR  = Path(os.environ.get("APPDATA", Path.home())) / "LiveOpsSentinel"
CFG_PATH = APP_DIR / "config.json"
LOG_PATH = APP_DIR / "gui.log"

DEFAULTS = {
    "webhook": "",
    "thresholds": {"rttMs": 80, "lossPct": 2.0, "holdSec": 5},
    "backend_path": "",
    "autostart_backend": True,
    "simpleMode": False,
    "theme": "dark",
    "current_bitrate_kbps": 6000
}

def setup_logging():
    """로깅 시스템 초기화 (최근 3개 파일 보존)"""
    APP_DIR.mkdir(parents=True, exist_ok=True)
    
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    
    # 기존 핸들러 제거
    for handler in logger.handlers[:]:
        logger.removeHandler(handler)
    
    # 파일 핸들러 (회전)
    file_handler = RotatingFileHandler(
        LOG_PATH, maxBytes=1024*1024, backupCount=3, encoding='utf-8'
    )
    file_handler.setFormatter(logging.Formatter(
        '%(asctime)s - %(levelname)s - %(message)s'
    ))
    logger.addHandler(file_handler)
    
    # 콘솔 핸들러
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(logging.Formatter(
        '%(levelname)s: %(message)s'
    ))
    logger.addHandler(console_handler)

def load() -> dict:
    try:
        if CFG_PATH.exists():
            return {**DEFAULTS, **json.loads(CFG_PATH.read_text(encoding="utf-8"))}
    except Exception as e:
        logging.error(f"설정 로드 실패: {e}")
    return DEFAULTS.copy()

def save(cfg: dict):
    try:
        APP_DIR.mkdir(parents=True, exist_ok=True)
        CFG_PATH.write_text(json.dumps(cfg, ensure_ascii=False, indent=2), encoding="utf-8")
        logging.info("설정 저장 완료")
    except Exception as e:
        logging.error(f"설정 저장 실패: {e}")
