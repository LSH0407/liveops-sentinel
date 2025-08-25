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
    
    def compute_quality(self, metrics_window: List[Dict], current_bitrate_kbps: int) -> Dict:
        """품질 점수 계산 및 권장사항 생성"""
        if not metrics_window:
            return self._default_result()
        
        # 최근 5초 평균 계산
        recent_metrics = self._calculate_recent_averages(metrics_window)
        
        # 각 지표별 점수 계산
        scores = {}
        reasons = []
        
        # RTT 점수
        rtt_score = self._normalize_rtt(recent_metrics.get('rtt_ms', 0))
        scores['rtt_ms'] = rtt_score
        if rtt_score < 60:
            reasons.append(f"서버 응답 속도 높음 ({recent_metrics.get('rtt_ms', 0):.1f}ms)")
        
        # Loss 점수
        loss_score = self._normalize_loss(recent_metrics.get('loss_pct', 0))
        scores['loss_pct'] = loss_score
        if loss_score < 60:
            reasons.append(f"전송 손실 높음 ({recent_metrics.get('loss_pct', 0):.2f}%)")
        
        # Uplink headroom 점수
        uplink_kbps = recent_metrics.get('uplink_kbps', 0)
        headroom_score = self._normalize_uplink_headroom(uplink_kbps, current_bitrate_kbps)
        scores['uplink_headroom'] = headroom_score
        if headroom_score < 60:
            headroom_pct = max(0, (uplink_kbps - current_bitrate_kbps) / current_bitrate_kbps * 100)
            reasons.append(f"업로드 대역폭 부족 (여유율 {headroom_pct:.1f}%)")
        
        # OBS 점수들
        dropped_score = self._normalize_dropped_ratio(recent_metrics.get('dropped_ratio', 0))
        scores['dropped_ratio'] = dropped_score
        if dropped_score < 60:
            reasons.append(f"버린 프레임 비율 높음 ({recent_metrics.get('dropped_ratio', 0)*100:.1f}%)")
        
        enc_lag_score = self._normalize_enc_lag(recent_metrics.get('enc_lag_ms', 0))
        scores['enc_lag_ms'] = enc_lag_score
        if enc_lag_score < 60:
            reasons.append(f"인코딩 지연 높음 ({recent_metrics.get('enc_lag_ms', 0):.1f}ms)")
        
        render_lag_score = self._normalize_render_lag(recent_metrics.get('render_lag_ms', 0))
        scores['render_lag_ms'] = render_lag_score
        if render_lag_score < 60:
            reasons.append(f"렌더 지연 높음 ({recent_metrics.get('render_lag_ms', 0):.1f}ms)")
        
        # 가중 평균 점수 계산
        total_score = sum(scores[key] * self.weights[key] for key in scores)
        
        # 상태 등급 결정
        grade = self._determine_grade(total_score)
        
        # 권장 조치 생성
        action = self._generate_action(recent_metrics, total_score, current_bitrate_kbps)
        
        return {
            'score': round(total_score, 1),
            'grade': grade,
            'reasons': reasons,
            'action': action,
            'details': scores
        }
    
    def _calculate_recent_averages(self, metrics_window: List[Dict]) -> Dict[str, float]:
        """최근 메트릭들의 평균값 계산"""
        if not metrics_window:
            return {}
        
        # 최근 5초 데이터만 사용
        recent_data = metrics_window[-5:] if len(metrics_window) > 5 else metrics_window
        
        averages = {}
        for metric in recent_data:
            for key, value in metric.items():
                if key not in averages:
                    averages[key] = []
                averages[key].append(value)
        
        return {key: sum(values) / len(values) for key, values in averages.items()}
    
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
    
    def _generate_action(self, metrics: Dict, score: float, current_bitrate_kbps: int) -> str:
        """권장 조치 생성"""
        if score >= 85:
            return "현재 설정이 최적입니다."
        
        actions = []
        
        # Loss 기반 권장
        loss_pct = metrics.get('loss_pct', 0)
        if loss_pct > 2:
            reduction = int(current_bitrate_kbps * 0.2)
            actions.append(f"비트레이트를 15~25% 낮추세요(예: {current_bitrate_kbps}→{current_bitrate_kbps - reduction}kbps).")
        
        # RTT 기반 권장
        rtt_ms = metrics.get('rtt_ms', 0)
        if rtt_ms > 100:
            actions.append("와이파이→유선 전환 권장. 공유기 QoS(업로드 우선순위) 확인.")
        
        # Dropped ratio 기반 권장
        dropped_ratio = metrics.get('dropped_ratio', 0)
        if dropped_ratio > 0.03:
            actions.append("출력 해상도 1080p60→720p60, 또는 인코더 NVENC로 전환.")
        
        # Enc/Render lag 기반 권장
        enc_lag = metrics.get('enc_lag_ms', 0)
        render_lag = metrics.get('render_lag_ms', 0)
        if enc_lag > 15 or render_lag > 20:
            actions.append("필요 시 필터/소스 수 줄이기. 캡처 소스 동시활성 최소화.")
        
        if not actions:
            return "네트워크 상태를 모니터링하고 필요시 설정을 조정하세요."
        
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
