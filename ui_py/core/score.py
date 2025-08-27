from typing import Dict, List, Tuple, Optional
import math

class QualityScore:
    """스트리밍 품질 점수 계산 및 권장사항 제공"""
    
    def __init__(self):
        # 가중치 설정 (합 = 1.0)
        self.weights = {
            'rtt_ms': 0.35,
            'loss_pct': 0.35,
            'uplink_headroom': 0.10,
            'dropped_ratio': 0.10,
            'enc_lag_ms': 0.05,
            'render_lag_ms': 0.05
        }
        
        # 상태 등급 기준
        self.grade_thresholds = {
            'good': 85,
            'warning': 60
        }
        
        # 용어 치환 (일반인 친화적)
        self.term_mapping = {
            'rtt_ms': '서버 응답 속도',
            'loss_pct': '전송 손실',
            'uplink_headroom': '업로드 여유율',
            'dropped_ratio': '버린 프레임 비율',
            'enc_lag_ms': '인코딩 지연',
            'render_lag_ms': '렌더 지연'
        }
    
    def compute_quality(self, metrics_window: List[dict], current_bitrate_kbps: int = 6000, obs_settings: dict = None) -> dict:
        """품질 점수 계산 및 권장사항 생성"""
        if not metrics_window:
            return {
                'score': 0,
                'grade': 'F',
                'action': '메트릭 데이터가 없습니다',
                'details': {}
            }
        
        # 최근 5개 메트릭 사용 (최대)
        recent_data = metrics_window[-5:] if len(metrics_window) >= 5 else metrics_window
        # print(f"품질 점수 계산: 최근 {len(recent_data)}개 메트릭 사용")
        
        # 평균값 계산
        result = self._calculate_recent_averages(recent_data)
        # print(f"품질 점수 계산: 평균값 = {result}")
        
        # 네트워크 점수 (40%)
        network_score = self._calculate_network_score(result, current_bitrate_kbps)
        # print(f"품질 점수 계산 상세: RTT={result.get('rtt_ms', 0)}, Loss={result.get('loss_pct', 0)}, Uplink={current_bitrate_kbps}")
        # print(f"품질 점수 계산 상세: CPU={result.get('cpu_pct', 0)}, GPU={result.get('gpu_pct', 0)}")
        # print(f"품질 점수 계산 상세: Dropped={result.get('dropped_ratio', 0)}, EncLag={result.get('enc_lag_ms', 0)}")
        
        # 시스템 점수 (30%)
        system_score = self._calculate_system_score(result)
        
        # OBS 점수 (30%)
        obs_score = self._calculate_obs_score(result)
        
        # 최종 점수 계산
        final_score = (network_score * 0.4) + (system_score * 0.3) + (obs_score * 0.3)
        
        # 등급 및 권장사항 결정
        grade, action = self._get_grade_and_action(final_score, result, current_bitrate_kbps, obs_settings)
        
        return {
            'score': round(final_score, 1),
            'grade': grade,
            'action': action,
            'details': {
                'network_score': round(network_score, 1),
                'system_score': round(system_score, 1),
                'obs_score': round(obs_score, 1),
                'avg_metrics': result
            }
        }
    
    def _calculate_recent_averages(self, metrics_window: List[Dict]) -> Dict[str, float]:
        """최근 메트릭들의 평균값 계산"""
        if not metrics_window:
            print("품질 점수 계산: metrics_window가 비어있음")
            return {}
        
        # 최근 5초 데이터만 사용
        recent_data = metrics_window[-5:] if len(metrics_window) > 5 else metrics_window
        print(f"품질 점수 계산: 최근 {len(recent_data)}개 메트릭 사용")
        
        # 각 메트릭별로 평균 계산
        metric_sums = {}
        metric_counts = {}
        
        for metric in recent_data:
            # print(f"처리 중인 메트릭: {metric}")
            for key, value in metric.items():
                if isinstance(value, (int, float)):  # 숫자만 처리
                    if key not in metric_sums:
                        metric_sums[key] = 0
                        metric_counts[key] = 0
                    metric_sums[key] += value
                    metric_counts[key] += 1
        
        result = {}
        for key in metric_sums:
            if metric_counts[key] > 0:
                result[key] = metric_sums[key] / metric_counts[key]
        
        print(f"품질 점수 계산: 평균값 = {result}")
        return result
    
    def _calculate_network_score(self, metrics: dict, current_bitrate_kbps: int) -> float:
        """네트워크 점수 계산 (40%)"""
        rtt_score = self._normalize_rtt(metrics.get('rtt_ms', 0))
        loss_score = self._normalize_loss(metrics.get('loss_pct', 0))
        uplink_score = self._normalize_uplink_headroom(metrics.get('uplink_kbps', 0), current_bitrate_kbps)
        
        # 가중 평균 (RTT 50%, Loss 30%, Uplink 20%)
        return (rtt_score * 0.5) + (loss_score * 0.3) + (uplink_score * 0.2)
    
    def _calculate_system_score(self, metrics: dict) -> float:
        """시스템 점수 계산 (30%)"""
        cpu_score = max(0, 100 - metrics.get('cpu_pct', 0))  # CPU 사용률이 낮을수록 좋음
        gpu_score = max(0, 100 - metrics.get('gpu_pct', 0))  # GPU 사용률이 낮을수록 좋음
        
        # CPU 60%, GPU 40%
        return (cpu_score * 0.6) + (gpu_score * 0.4)
    
    def _calculate_obs_score(self, metrics: dict) -> float:
        """OBS 점수 계산 (30%)"""
        dropped_score = self._normalize_dropped_ratio(metrics.get('dropped_ratio', 0))
        enc_lag_score = self._normalize_enc_lag(metrics.get('enc_lag_ms', 0))
        render_lag_score = self._normalize_render_lag(metrics.get('render_lag_ms', 0))
        
        # 가중 평균 (Dropped 50%, Enc Lag 25%, Render Lag 25%)
        return (dropped_score * 0.5) + (enc_lag_score * 0.25) + (render_lag_score * 0.25)
    
    def _get_grade_and_action(self, score: float, metrics: dict, current_bitrate_kbps: int, obs_settings: dict = None) -> tuple:
        """등급과 권장사항 결정"""
        if score >= 85:
            grade = "좋음"
        elif score >= 60:
            grade = "주의"
        else:
            grade = "불안정"
        
        # 개선된 권장사항 생성 메서드 사용
        action = self._generate_action(metrics, score, current_bitrate_kbps)
        
        return grade, action
    
    def _normalize_rtt(self, rtt_ms: float) -> float:
        """RTT 정규화 (20ms=100, 80ms=60, 150ms=30, 300ms=0)"""
        if rtt_ms <= 20:
            return 100
        elif rtt_ms <= 80:
            return 100 - (rtt_ms - 20) * 40 / 60
        elif rtt_ms <= 150:
            return 60 - (rtt_ms - 80) * 30 / 70
        else:
            return max(0, 30 - (rtt_ms - 150) * 30 / 150)
    
    def _normalize_loss(self, loss_pct: float) -> float:
        """Loss 정규화 (0%=100, 0.5%=80, 2%=40, 5%=0)"""
        if loss_pct <= 0:
            return 100
        elif loss_pct <= 0.5:
            return 100 - loss_pct * 40 / 0.5
        elif loss_pct <= 2:
            return 80 - (loss_pct - 0.5) * 40 / 1.5
        else:
            return max(0, 40 - (loss_pct - 2) * 40 / 3)
    
    def _normalize_uplink_headroom(self, uplink_kbps: float, current_bitrate_kbps: int) -> float:
        """업로드 여유율 정규화"""
        if current_bitrate_kbps <= 0:
            return 100
        
        headroom_pct = (uplink_kbps - current_bitrate_kbps) / current_bitrate_kbps * 100
        
        if headroom_pct >= 50:
            return 100
        elif headroom_pct >= 20:
            return 70 + (headroom_pct - 20) * 30 / 30
        elif headroom_pct >= 0:
            return 40 + headroom_pct * 30 / 20
        else:
            return max(10, 40 + headroom_pct * 30 / 20)
    
    def _normalize_dropped_ratio(self, dropped_ratio: float) -> float:
        """Dropped ratio 정규화 (0%=100, 1%=70, 3%=30, 5%=0)"""
        dropped_pct = dropped_ratio * 100
        if dropped_pct <= 0:
            return 100
        elif dropped_pct <= 1:
            return 100 - dropped_pct * 30 / 1
        elif dropped_pct <= 3:
            return 70 - (dropped_pct - 1) * 40 / 2
        else:
            return max(0, 30 - (dropped_pct - 3) * 30 / 2)
    
    def _normalize_enc_lag(self, enc_lag_ms: float) -> float:
        """인코딩 지연 정규화 (0~5ms=100, 10ms=70, 20ms=30, 40ms=0)"""
        if enc_lag_ms <= 5:
            return 100
        elif enc_lag_ms <= 10:
            return 100 - (enc_lag_ms - 5) * 30 / 5
        elif enc_lag_ms <= 20:
            return 70 - (enc_lag_ms - 10) * 40 / 10
        else:
            return max(0, 30 - (enc_lag_ms - 20) * 30 / 20)
    
    def _normalize_render_lag(self, render_lag_ms: float) -> float:
        """렌더 지연 정규화 (0~7ms=100, 14ms=70, 25ms=30, 40ms=0)"""
        if render_lag_ms <= 7:
            return 100
        elif render_lag_ms <= 14:
            return 100 - (render_lag_ms - 7) * 30 / 7
        elif render_lag_ms <= 25:
            return 70 - (render_lag_ms - 14) * 40 / 11
        else:
            return max(0, 30 - (render_lag_ms - 25) * 30 / 15)
    
    def _determine_grade(self, score: float) -> str:
        """점수에 따른 상태 등급 결정"""
        if score >= self.grade_thresholds['good']:
            return "좋음"
        elif score >= self.grade_thresholds['warning']:
            return "주의"
        else:
            return "불안정"
    
    def _generate_action(self, metrics: Dict, score: float, current_bitrate_kbps: int, obs_settings: dict = None) -> str:
        """권장 조치 생성"""
        if score >= 85:
            return "현재 설정이 최적입니다. 지속적으로 모니터링하세요."
        
        actions = []
        
        # Loss 기반 권장
        loss_pct = metrics.get('loss_pct', 0)
        if loss_pct > 2:
            reduction = int(current_bitrate_kbps * 0.2)
            actions.append(f"비트레이트를 20% 낮추세요({current_bitrate_kbps}→{current_bitrate_kbps - reduction}kbps).")
        elif loss_pct > 0.5:
            actions.append("네트워크 상태를 모니터링하고 필요시 비트레이트를 조정하세요.")
        
        # RTT 기반 권장
        rtt_ms = metrics.get('rtt_ms', 0)
        if rtt_ms > 100:
            actions.append("와이파이→유선 전환 권장. 공유기 QoS 설정을 확인하세요.")
        elif rtt_ms > 80:
            actions.append("네트워크 연결 상태를 점검하고 필요시 공유기 설정을 확인하세요.")
        
        # Dropped ratio 기반 권장 (OBS 설정 고려)
        dropped_ratio = metrics.get('dropped_ratio', 0)
        if dropped_ratio > 0.03:
            if obs_settings:
                current_resolution = obs_settings.get('output_resolution', 'Unknown')
                current_encoder = obs_settings.get('encoder', 'Unknown')
                if '1920x1080' in current_resolution:
                    actions.append(f"현재 해상도({current_resolution})가 높습니다. 720p60으로 낮추세요.")
                elif 'x264' in current_encoder:
                    actions.append(f"현재 인코더({current_encoder})가 CPU 부하가 높습니다. NVENC으로 전환하세요.")
                else:
                    actions.append("출력 해상도를 720p60으로 낮추거나 NVENC 인코더로 전환하세요.")
            else:
                actions.append("출력 해상도를 720p60으로 낮추거나 NVENC 인코더로 전환하세요.")
        elif dropped_ratio > 0.01:
            actions.append("OBS 설정을 최적화하고 필요시 해상도를 조정하세요.")
        
        # Enc/Render lag 기반 권장
        enc_lag = metrics.get('enc_lag_ms', 0)
        render_lag = metrics.get('render_lag_ms', 0)
        if enc_lag > 15 or render_lag > 20:
            actions.append("필터나 소스 수를 줄이고 캡처 소스 동시활성을 최소화하세요.")
        elif enc_lag > 10 or render_lag > 14:
            actions.append("OBS 성능을 최적화하고 필요시 설정을 조정하세요.")
        
        # CPU/GPU 기반 권장
        cpu_pct = metrics.get('cpu_pct', 0)
        gpu_pct = metrics.get('gpu_pct', 0)
        if cpu_pct > 80:
            actions.append("CPU 사용률이 높습니다. 해상도를 낮추거나 HW 인코더를 사용하세요.")
        elif gpu_pct > 85:
            actions.append("GPU 사용률이 높습니다. 그래픽 설정을 최적화하세요.")
        
        if not actions:
            return "현재 상태가 양호합니다. 지속적으로 모니터링하세요."
        
        return " ".join(actions[:2])  # 최대 2개 권장사항만 표시
    
    def _default_result(self) -> Dict:
        """기본 결과 (데이터 없을 때)"""
        return {
            'score': 0,
            'grade': '불안정',
            'reasons': ['데이터 없음'],
            'action': '백엔드 연결을 확인하세요.',
            'details': {}
        }
    
    def get_term_name(self, key: str) -> str:
        """일반인 친화적 용어 반환"""
        return self.term_mapping.get(key, key)
