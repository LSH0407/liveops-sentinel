"""
포터블 빌드 스크립트
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from datetime import datetime

def build_portable():
    """포터블 빌드 실행"""
    
    # 프로젝트 루트 경로
    project_root = Path(__file__).parent.parent.parent
    ui_py_path = project_root / "ui_py"
    packaging_path = ui_py_path / "packaging"
    
    print("🚀 LiveOps Sentinel 포터블 빌드 시작...")
    
    # 1. Python 의존성 설치 확인
    print("📦 Python 의존성 확인 중...")
    try:
        subprocess.run([sys.executable, "-m", "pip", "install", "-r", str(ui_py_path / "requirements.txt")], 
                      check=True, capture_output=True)
        print("✅ 의존성 설치 완료")
    except subprocess.CalledProcessError as e:
        print(f"❌ 의존성 설치 실패: {e}")
        return False
    
    # 2. 백엔드 빌드 확인
    backend_path = project_root / "build" / "Release" / "liveops_backend.exe"
    if not backend_path.exists():
        print("⚠️  백엔드 실행 파일이 없습니다. C++ 백엔드를 먼저 빌드하세요.")
        print("   cmake -B build -S . -G \"Visual Studio 17 2022\" -A x64")
        print("   cmake --build build --config Release --parallel")
        return False
    
    print(f"✅ 백엔드 실행 파일 확인: {backend_path}")
    
    # 3. PyInstaller 빌드
    print("🔨 PyInstaller 빌드 중...")
    spec_file = packaging_path / "liveops_gui.spec"
    
    try:
        result = subprocess.run([
            sys.executable, "-m", "PyInstaller",
            "--clean",
            str(spec_file)
        ], cwd=ui_py_path, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ PyInstaller 빌드 완료")
        else:
            print(f"❌ PyInstaller 빌드 실패:")
            print(result.stderr)
            return False
            
    except Exception as e:
        print(f"❌ PyInstaller 실행 오류: {e}")
        return False
    
    # 4. 빌드 결과 확인
    dist_path = ui_py_path / "dist" / "LiveOpsSentinel"
    if not dist_path.exists():
        print("❌ 빌드 결과 폴더가 없습니다.")
        return False
    
    # 5. 실행 파일 확인
    exe_path = dist_path / "LiveOpsSentinel.exe"
    if not exe_path.exists():
        print("❌ 실행 파일이 생성되지 않았습니다.")
        return False
    
    print(f"✅ 실행 파일 생성: {exe_path}")
    
    # 6. 파일 크기 및 구성 확인
    total_size = sum(f.stat().st_size for f in dist_path.rglob('*') if f.is_file())
    print(f"📊 빌드 크기: {total_size / (1024*1024):.1f} MB")
    
    # 7. ZIP 패키지 생성 (선택사항)
    create_zip = input("\n📦 ZIP 패키지를 생성하시겠습니까? (y/N): ").lower().strip()
    if create_zip == 'y':
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        zip_name = f"LiveOpsSentinel_Portable_{timestamp}.zip"
        zip_path = project_root / zip_name
        
        print(f"📦 ZIP 패키지 생성 중: {zip_path}")
        
        try:
            shutil.make_archive(str(zip_path.with_suffix('')), 'zip', dist_path)
            print(f"✅ ZIP 패키지 생성 완료: {zip_path}")
        except Exception as e:
            print(f"❌ ZIP 패키지 생성 실패: {e}")
    
    print("\n🎉 포터블 빌드 완료!")
    print(f"📁 실행 파일 위치: {exe_path}")
    print(f"📁 전체 패키지: {dist_path}")
    
    return True

def clean_build():
    """빌드 정리"""
    ui_py_path = Path(__file__).parent.parent
    
    print("🧹 빌드 파일 정리 중...")
    
    # PyInstaller 빌드 파일 정리
    build_dirs = [
        ui_py_path / "build",
        ui_py_path / "dist",
        ui_py_path / "__pycache__",
    ]
    
    for build_dir in build_dirs:
        if build_dir.exists():
            try:
                shutil.rmtree(build_dir)
                print(f"✅ 정리 완료: {build_dir}")
            except Exception as e:
                print(f"⚠️  정리 실패: {build_dir} - {e}")
    
    # .spec 파일 정리
    spec_files = list(ui_py_path.glob("*.spec"))
    for spec_file in spec_files:
        try:
            spec_file.unlink()
            print(f"✅ 정리 완료: {spec_file}")
        except Exception as e:
            print(f"⚠️  정리 실패: {spec_file} - {e}")
    
    print("✅ 빌드 정리 완료")

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="LiveOps Sentinel 포터블 빌드")
    parser.add_argument("--clean", action="store_true", help="빌드 파일 정리")
    
    args = parser.parse_args()
    
    if args.clean:
        clean_build()
    else:
        build_portable()
