"""
ZIP 내보내기 기능
"""

import zipfile
import os
from datetime import datetime
from pathlib import Path
from typing import List, Dict
import json

class ZipExporter:
    """ZIP 내보내기 클래스"""
    
    def __init__(self):
        pass
    
    def create_report_zip(self, session_path: Path, platform: str, duration_seconds: int) -> Path:
        """진단 리포트 ZIP 파일 생성"""
        
        # ZIP 파일명 생성
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        session_id = f"session_{timestamp}"
        zip_filename = f"LiveOpsReport_{timestamp}_{session_id}.zip"
        
        # ZIP 파일 경로
        documents_path = Path.home() / "Documents" / "LiveOpsReports"
        zip_path = documents_path / zip_filename
        
        # ZIP 파일 생성
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            # 세션 폴더의 모든 파일을 ZIP에 추가
            self._add_directory_to_zip(zipf, session_path, session_path.name)
            
            # README 파일 추가
            readme_content = self._generate_readme(platform, duration_seconds, session_path)
            zipf.writestr("README.txt", readme_content)
            
            # 설정 파일 추가 (민감정보 제거)
            config_content = self._generate_redacted_config()
            zipf.writestr("redacted_config.json", config_content)
        
        return zip_path
    
    def _add_directory_to_zip(self, zipf: zipfile.ZipFile, directory: Path, arcname: str):
        """디렉토리의 모든 파일을 ZIP에 추가"""
        for file_path in directory.rglob("*"):
            if file_path.is_file():
                # 상대 경로 계산
                relative_path = file_path.relative_to(directory)
                arc_path = f"{arcname}/{relative_path}"
                
                # 파일을 ZIP에 추가
                zipf.write(file_path, arc_path)
    
    def _generate_readme(self, platform: str, duration_seconds: int, session_path: Path) -> str:
        """README 파일 내용 생성"""
        
        # 통계 파일 읽기
        stats_file = session_path / "diagnostic_report.json"
        stats_info = ""
        
        if stats_file.exists():
            try:
                with open(stats_file, 'r', encoding='utf-8') as f:
                    report_data = json.load(f)
                
                stats = report_data.get('statistics', {})
                stats_info = f"""
📊 진단 통계:
• 응답시간: 평균 {stats.get('rtt_ms', {}).get('avg', 0):.1f}ms (최대 {stats.get('rtt_ms', {}).get('max', 0):.1f}ms)
• 손실률: 평균 {stats.get('loss_pct', {}).get('avg', 0):.2f}% (최대 {stats.get('loss_pct', {}).get('max', 0):.2f}%)
• 업로드: 평균 {stats.get('uplink_kbps', {}).get('avg', 0):.0f}kbps (최대 {stats.get('uplink_kbps', {}).get('max', 0):.0f}kbps)
• CPU: 평균 {stats.get('cpu_pct', {}).get('avg', 0):.1f}% (최대 {stats.get('cpu_pct', {}).get('max', 0):.1f}%)
• 메모리: 평균 {stats.get('memory_pct', {}).get('avg', 0):.1f}% (최대 {stats.get('memory_pct', {}).get('max', 0):.1f}%)
• 총 샘플: {report_data.get('total_samples', 0)}개
            """
            except Exception as e:
                stats_info = f"통계 정보를 읽을 수 없습니다: {e}"
        
        readme_content = f"""
LiveOps Sentinel 진단 리포트
============================

📋 진단 정보:
• 플랫폼: {platform}
• 진단 시간: {duration_seconds}초
• 생성 시간: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
• 세션 ID: {session_path.name}

{stats_info}

📁 파일 구성:
• report.html - HTML 형식 진단 리포트
• diagnostic_report.json - JSON 형식 상세 데이터
• metrics.csv - CSV 형식 메트릭 데이터
• redacted_config.json - 설정 정보 (민감정보 제거)

💡 사용법:
1. report.html을 웹 브라우저에서 열어 상세 리포트 확인
2. metrics.csv를 Excel에서 열어 데이터 분석
3. diagnostic_report.json을 프로그래밍 도구에서 활용

🔧 권장사항:
• 응답시간이 100ms를 초과하는 경우 유선 연결 사용
• 패킷 손실이 2%를 초과하는 경우 비트레이트 20% 낮추기
• 업로드 대역폭의 70% 이하로 비트레이트 설정
• CPU 사용률이 80%를 초과하는 경우 해상도 낮추기

📞 지원:
• GitHub Issues: https://github.com/your-username/liveops-sentinel/issues
• 문서: https://github.com/your-username/liveops-sentinel/wiki

---
LiveOps Sentinel - 안정적인 스트리밍을 위한 스마트 모니터링 솔루션
        """
        
        return readme_content
    
    def _generate_redacted_config(self) -> str:
        """민감정보가 제거된 설정 파일 생성"""
        
        # 현재 설정 로드
        try:
            from settings import load
            config = load()
            
            # 민감정보 제거
            redacted_config = {
                "platform": config.get("platform", "unknown"),
                "thresholds": config.get("thresholds", {}),
                "ui": {
                    "theme": config.get("ui", {}).get("theme", "dark"),
                    "simpleMode": config.get("ui", {}).get("simpleMode", True)
                },
                "diagnostic": {
                    "default_duration_minutes": config.get("diag_minutes", 60)
                }
            }
            
            return json.dumps(redacted_config, indent=2, ensure_ascii=False)
            
        except Exception as e:
            return json.dumps({"error": f"설정을 읽을 수 없습니다: {e}"}, indent=2, ensure_ascii=False)
    
    def create_custom_zip(self, files: List[Path], zip_filename: str, base_path: Path = None) -> Path:
        """사용자 지정 파일들로 ZIP 생성"""
        
        # ZIP 파일 경로
        documents_path = Path.home() / "Documents" / "LiveOpsReports"
        zip_path = documents_path / zip_filename
        
        # ZIP 파일 생성
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for file_path in files:
                if file_path.exists():
                    if base_path:
                        # 상대 경로로 추가
                        arcname = file_path.relative_to(base_path)
                    else:
                        # 파일명만 사용
                        arcname = file_path.name
                    
                    zipf.write(file_path, arcname)
        
        return zip_path
