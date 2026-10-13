"""격리된 새 로비 서버에서 최종 입력 처리와 대기열 정리를 관측한다."""
import asyncio
import copy
import json
import time


class DrainCheck:
    def __init__(self, since_ms, expected_packets):
        self.since_ms = since_ms
        self.expected_packets = expected_packets
        self.instance = None
        self.sequence = None
        self.clean_samples = 0
        self.last = {}

    def observe(self, row, now_ms):
        stamp = row.get('timestamp_ms')
        if row.get('status') != 'live' or type(stamp) is not int or stamp < self.since_ms or not -2000 <= now_ms - stamp <= 3500:
            self.clean_samples = 0
            return False
        instance, sequence = row.get('instance'), row.get('sequence')
        if not instance or type(sequence) is not int:
            self.clean_samples = 0
            return False
        if self.instance is not None and instance != self.instance:
            raise ValueError('대기열 정리 중 서버 재시작')
        self.instance = instance
        if sequence == self.sequence:
            return False
        if self.sequence is not None and sequence < self.sequence:
            raise ValueError('대기열 정리 중 reporter sequence 감소')
        self.sequence = sequence
        metrics = row.get('performance', {})
        self.last = {'timestamp_ms': stamp, 'sequence': sequence,
                     'processed_packets': metrics.get('packets_processed'),
                     'pending_packets': metrics.get('pending_packets', {}).get('count'),
                     'queued_sends': row.get('send_queue', {}).get('count')}
        values = [self.last[key] for key in ('processed_packets', 'pending_packets', 'queued_sends')]
        clean = all(type(value) is int and value >= 0 for value in values) and values[0] >= self.expected_packets and values[1:] == [0, 0]
        self.clean_samples = self.clean_samples + 1 if clean else 0
        return self.clean_samples >= 3


async def wait_for_drain(state, timeout_s, poll_s=.2):
    """다른 클라이언트가 없는 새 로비 서버만 지원. TCP 애플리케이션별 ACK는 아니다."""
    start = time.monotonic()
    with state.lock:
        check = DrainCheck(time.time_ns() // 1_000_000, state.counters['sent_packets'])
    state.event(-1, 'server-drain', 'start')
    status, detail = 'failed', ''
    try:
        while time.monotonic() - start < timeout_s:
            with state.lock:
                row = copy.deepcopy(state.server_observation.get('LobbyServer', {}))
            if check.observe(row, time.time_ns() // 1_000_000):
                status = 'passed'
                return
            await asyncio.sleep(poll_s)
        raise TimeoutError('최종 입력 처리/패킷·송신 대기열 정리 timeout: ' + json.dumps(check.last, ensure_ascii=False))
    except asyncio.CancelledError:
        status, detail = 'cancelled', 'CancelledError'
        raise
    except Exception as error:
        detail = str(error)
        raise
    finally:
        elapsed_ms = (time.monotonic() - start) * 1000
        with state.lock:
            state.settlement = {'status': status, 'elapsed_ms': elapsed_ms, 'expected_packets': check.expected_packets,
                                'clean_samples': check.clean_samples, 'last': check.last, 'error': detail or None}
        state.event(-1, 'server-drain', {'passed': 'pass', 'failed': 'fail', 'cancelled': 'cancelled'}[status], detail, elapsed_ms)
