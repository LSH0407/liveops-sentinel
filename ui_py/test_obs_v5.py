#!/usr/bin/env python3
"""
OBS WebSocket v5 연결 테스트 스크립트
"""

import sys
import time

def test_obs_v5_connection(host="127.0.0.1", port=4455, password="", use_tls=False):
    """OBS WebSocket v5 연결 테스트"""
    print("OBS WebSocket v5 연결 테스트")
    print("=" * 50)
    print(f"호스트: {host}")
    print(f"포트: {port}")
    print(f"TLS: {use_tls}")
    print(f"비밀번호: {'*' * len(password) if password else '없음'}")
    print()
    
    try:
        from obsws_python import ReqClient, error as obs_err
        
        print("1. OBS WebSocket v5 클라이언트 생성 중...")
        
        # TLS 설정에 따른 연결
        if use_tls:
            cli = ReqClient(
                host=host, 
                port=port, 
                password=password,
                timeout=3.0, 
                url=f"wss://{host}"
            )
        else:
            cli = ReqClient(
                host=host, 
                port=port, 
                password=password,
                timeout=3.0
            )
        
        print("✅ OBS WebSocket v5 클라이언트 생성 성공")
        
        print("\n2. OBS 버전 정보 조회 중...")
        version = cli.get_version()
        print(f"✅ OBS 버전: {version.obs_web_socket_version}")
        print(f"   OBS Studio 버전: {version.obs_version}")
        print(f"   플랫폼: {version.platform}")
        
        print("\n3. 스트림 상태 조회 중...")
        try:
            stats = cli.get_stream_status()
            print(f"✅ 스트림 상태: {'실행 중' if stats.output_active else '중지됨'}")
            if stats.output_active:
                print(f"   드롭된 프레임: {stats.dropped_frames}")
                print(f"   총 프레임: {stats.total_frames}")
                print(f"   인코딩 지연: {stats.encoding_lag}ms")
                print(f"   렌더 지연: {stats.render_lag}ms")
                print(f"   FPS: {stats.fps}")
                print(f"   비트레이트: {stats.bitrate / 1000:.1f} kbps")
        except Exception as e:
            print(f"⚠️ 스트림 상태 조회 실패: {e}")
        
        print("\n4. 연결 종료 중...")
        cli.disconnect()
        print("✅ 연결 종료 완료")
        
        print("\n🎉 OBS WebSocket v5 연결 테스트 성공!")
        return True
        
    except ImportError:
        print("❌ obsws-python이 설치되지 않았습니다.")
        print("   설치 명령: pip install obsws-python>=1.6.0")
        return False
        
    except obs_err.OBSSDKError as e:
        if "authentication enabled but no password provided" in str(e):
            print(f"❌ 인증 실패: {e}")
            print("   OBS Studio에서 인증이 활성화되어 있지만 비밀번호가 제공되지 않음")
            print("   OBS Studio에서 Tools > WebSocket Server Settings에서 인증을 비활성화하거나")
            print("   비밀번호를 설정하고 테스트 스크립트에 전달하세요")
        else:
            print(f"❌ OBS SDK 오류: {e}")
        return False
        
    except Exception as e:
        print(f"❌ 예외 발생: {type(e).__name__}: {e}")
        return False

def main():
    """메인 함수"""
    # 기본 설정
    host = "127.0.0.1"
    port = 4455
    password = ""
    use_tls = False
    
    # 명령행 인수 처리
    if len(sys.argv) > 1:
        host = sys.argv[1]
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    if len(sys.argv) > 3:
        password = sys.argv[3]
    if len(sys.argv) > 4:
        use_tls = sys.argv[4].lower() in ['true', '1', 'yes', 'on']
    
    # 테스트 실행
    result = test_obs_v5_connection(host, port, password, use_tls)
    
    if result:
        print("\n✅ OBS WebSocket v5 연결이 정상적으로 작동합니다!")
        sys.exit(0)
    else:
        print("\n💥 OBS WebSocket v5 연결에 실패했습니다.")
        print("\n확인사항:")
        print("1. OBS Studio가 실행 중인가요?")
        print("2. Tools > WebSocket Server Settings에서 서버가 활성화되어 있나요?")
        print("3. 포트 번호가 올바른가요? (기본값: 4455)")
        print("4. TLS 설정이 OBS Studio와 일치하나요?")
        print("5. 비밀번호가 설정되어 있다면 올바른가요? (끝 공백 주의)")
        sys.exit(1)

if __name__ == "__main__":
    main()
