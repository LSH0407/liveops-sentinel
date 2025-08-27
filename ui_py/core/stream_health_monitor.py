from typing import Dict, List, Optional, Callable
from PySide6.QtCore import QObject, Signal, QTimer
import time
import math

class StreamHealthMonitor(QObject):
    """스트리밍 끊김 감지 및 알림 시스템"""
    
    # 시그널 정의
    stream_interruption_detected = Signal(str)  # 끊김 감지 시
    stream_quality_degraded = Signal(str)       # 품질 저하 시
    stream_recovered = Signal()                 # 복구 시
    
    def __init__(self):
        super().__init__()
        
        # 임계값 설정
        self.thresholds = {
            'dropped_frames_critical': 0.05,    # 5% 이상 드롭 시 끊김으로 판단
            'dropped_frames_warning': 0.02,     # 2% 이상 드롭 시 경고
            'rtt_critical': 200,                # 200ms 이상 RTT 시 끊김 가능성
            'rtt_warning': 150,                 # 150ms 이상 RTT 시 경고
            'loss_critical': 0.05,              # 5% 이상 손실 시 끊김
            'loss_warning': 0.02,               # 2% 이상 손실 시 경고
            'enc_lag_critical': 50,             # 50ms 이상 인코딩 지연 시 끊김
            'render_lag_critical': 50,          # 50ms 이상 렌더 지연 시 끊김
        }
        
        # 상태 추적
        self.current_status = "normal"  # normal, warning, critical
        self.interruption_start_time = None
        self.recovery_start_time = None
        self.consecutive_bad_samples = 0
        self.consecutive_good_samples = 0
        
        # 복구 판정 기준
        self.recovery_threshold = 10  # 연속 10개 좋은 샘플 시 복구로 판단
        self.critical_threshold = 5   # 연속 5개 나쁜 샘플 시 끊김으로 판단
        
        # 메트릭 히스토리 (최근 30초)
        self.metrics_history = []
        self.max_history_size = 30
        
        # 알림 콜백
        self.notification_callback: Optional[Callable] = None
        
        # 모니터링 타이머
        self.monitor_timer = QTimer()
        self.monitor_timer.timeout.connect(self._check_stream_health)
        self.monitor_timer.start(1000)  # 1초마다 체크
    
    def set_notification_callback(self, callback: Callable):
        """알림 콜백 설정"""
        self.notification_callback = callback
    
    def update_metrics(self, metrics: Dict):
        """새로운 메트릭 업데이트"""
        timestamp = time.time()
        
        # 메트릭 히스토리에 추가
        self.metrics_history.append({
            'timestamp': timestamp,
            'metrics': metrics.copy()
        })
        
        # 히스토리 크기 제한
        if len(self.metrics_history) > self.max_history_size:
            self.metrics_history.pop(0)
        
        # 스트림 헬스 체크
        self._analyze_stream_health(metrics)
    
    def _analyze_stream_health(self, metrics: Dict):
        """스트림 헬스 분석"""
        # 네트워크 메트릭 추출
        net_metrics = metrics.get('net', {})
        obs_metrics = metrics.get('obs', {})
        
        rtt_ms = net_metrics.get('rtt_ms', 0)
        loss_pct = net_metrics.get('loss_pct', 0)
        dropped_ratio = obs_metrics.get('dropped_ratio', 0)
        enc_lag_ms = obs_metrics.get('enc_lag_ms', 0)
        render_lag_ms = obs_metrics.get('render_lag_ms', 0)
        
        # 끊김 지표 계산
        interruption_score = self._calculate_interruption_score(
            rtt_ms, loss_pct, dropped_ratio, enc_lag_ms, render_lag_ms
        )
        
        # 상태 업데이트
        self._update_status(interruption_score)
    
    def _calculate_interruption_score(self, rtt_ms: float, loss_pct: float, 
                                    dropped_ratio: float, enc_lag_ms: float, 
                                    render_lag_ms: float) -> float:
        """끊김 점수 계산 (0-100, 높을수록 끊김 가능성 높음)"""
        score = 0
        
        # 드롭된 프레임 비율 (가장 중요)
        if dropped_ratio > self.thresholds['dropped_frames_critical']:
            score += 40
        elif dropped_ratio > self.thresholds['dropped_frames_warning']:
            score += 20
        
        # 패킷 손실
        if loss_pct > self.thresholds['loss_critical']:
            score += 30
        elif loss_pct > self.thresholds['loss_warning']:
            score += 15
        
        # RTT 지연
        if rtt_ms > self.thresholds['rtt_critical']:
            score += 20
        elif rtt_ms > self.thresholds['rtt_warning']:
            score += 10
        
        # 인코딩/렌더 지연
        if enc_lag_ms > self.thresholds['enc_lag_critical']:
            score += 10
        if render_lag_ms > self.thresholds['render_lag_critical']:
            score += 10
        
        return min(100, score)
    
    def _update_status(self, interruption_score: float):
        """상태 업데이트"""
        if interruption_score >= 70:  # 심각한 끊김
            self.consecutive_bad_samples += 1
            self.consecutive_good_samples = 0
            
            if self.consecutive_bad_samples >= self.critical_threshold:
                if self.current_status != "critical":
                    self.current_status = "critical"
                    self.interruption_start_time = time.time()
                    self._trigger_interruption_alert(interruption_score)
        
        elif interruption_score >= 40:  # 경고 수준
            self.consecutive_bad_samples += 1
            self.consecutive_good_samples = 0
            
            if self.consecutive_bad_samples >= self.critical_threshold:
                if self.current_status != "warning":
                    self.current_status = "warning"
                    self._trigger_quality_degradation_alert(interruption_score)
        
        else:  # 정상
            self.consecutive_good_samples += 1
            self.consecutive_bad_samples = 0
            
            if self.consecutive_good_samples >= self.recovery_threshold:
                if self.current_status != "normal":
                    self.current_status = "normal"
                    self.recovery_start_time = time.time()
                    self._trigger_recovery_alert()
    
    def _trigger_interruption_alert(self, score: float):
        """끊김 알림 트리거"""
        duration = self._get_interruption_duration()
        
        if duration < 10:  # 10초 미만은 일시적 끊김
            message = "⚠️ 일시적인 스트리밍 끊김이 감지되었습니다."
        elif duration < 30:  # 30초 미만은 중간 끊김
            message = "🚨 스트리밍이 불안정합니다. 시청자 경험에 영향을 줄 수 있습니다."
        else:  # 30초 이상은 심각한 끊김
            message = "💥 심각한 스트리밍 끊김이 발생했습니다. 즉시 조치가 필요합니다."
        
        # 시그널 발생
        self.stream_interruption_detected.emit(message)
        
        # 콜백 호출
        if self.notification_callback:
            self.notification_callback("stream_interruption", message)
    
    def _trigger_quality_degradation_alert(self, score: float):
        """품질 저하 알림 트리거"""
        message = "📉 스트리밍 품질이 저하되고 있습니다. 모니터링을 강화하세요."
        
        # 시그널 발생
        self.stream_quality_degraded.emit(message)
        
        # 콜백 호출
        if self.notification_callback:
            self.notification_callback("quality_degradation", message)
    
    def _trigger_recovery_alert(self):
        """복구 알림 트리거"""
        duration = self._get_interruption_duration()
        
        if duration > 30:
            message = "✅ 스트리밍이 안정적으로 복구되었습니다."
        else:
            message = "✅ 스트리밍 상태가 정상으로 돌아왔습니다."
        
        # 시그널 발생
        self.stream_recovered.emit()
        
        # 콜백 호출
        if self.notification_callback:
            self.notification_callback("stream_recovered", message)
    
    def _get_interruption_duration(self) -> float:
        """끊김 지속 시간 계산"""
        if self.interruption_start_time is None:
            return 0
        return time.time() - self.interruption_start_time
    
    def _check_stream_health(self):
        """정기적인 스트림 헬스 체크"""
        if not self.metrics_history:
            return
        
        # 최근 메트릭으로 헬스 체크
        latest_metrics = self.metrics_history[-1]['metrics']
        self._analyze_stream_health(latest_metrics)
    
    def get_current_status(self) -> Dict:
        """현재 상태 정보 반환"""
        return {
            'status': self.current_status,
            'interruption_duration': self._get_interruption_duration(),
            'consecutive_bad_samples': self.consecutive_bad_samples,
            'consecutive_good_samples': self.consecutive_good_samples,
            'metrics_history_size': len(self.metrics_history)
        }
    
    def get_stream_health_summary(self) -> Dict:
        """스트림 헬스 요약 정보"""
        if not self.metrics_history:
            return {'status': 'no_data', 'message': '메트릭 데이터가 없습니다.'}
        
        # 최근 10개 메트릭 분석
        recent_metrics = self.metrics_history[-10:]
        
        # 평균값 계산
        avg_rtt = sum(m['metrics'].get('net', {}).get('rtt_ms', 0) for m in recent_metrics) / len(recent_metrics)
        avg_loss = sum(m['metrics'].get('net', {}).get('loss_pct', 0) for m in recent_metrics) / len(recent_metrics)
        avg_dropped = sum(m['metrics'].get('obs', {}).get('dropped_ratio', 0) for m in recent_metrics) / len(recent_metrics)
        
        # 헬스 점수 계산
        health_score = 100 - self._calculate_interruption_score(
            avg_rtt, avg_loss, avg_dropped, 0, 0
        )
        
        return {
            'status': self.current_status,
            'health_score': max(0, health_score),
            'avg_rtt_ms': round(avg_rtt, 1),
            'avg_loss_pct': round(avg_loss, 2),
            'avg_dropped_ratio': round(avg_dropped, 3),
            'interruption_duration': self._get_interruption_duration()
        }
