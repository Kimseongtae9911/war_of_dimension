"""실제 서버 부하 구간과 누적 histogram 차분. percentile은 bucket 상한 근사다."""
import asyncio
import math
import random
import time

HISTOGRAMS = ('job_wait_us', 'packet_job_wait_us', 'tick_delay_us', 'tick_duration_us', 'lobby_movement_gap_us')
OPTIONAL_HISTOGRAMS = ('tick_timer_lateness_us', 'tick_dispatch_wait_us')


def histogram_delta(before, after):
    if not before or not after:
        return None
    left, right = before.get('buckets'), after.get('buckets')
    if not isinstance(left, list) or not isinstance(right, list) or len(left) != 33 or len(right) != 33:
        raise ValueError('histogram bucket 형식 불일치')
    if any(type(value) is not int or value < 0 for value in left + right):
        raise ValueError('histogram bucket 값 불일치')
    counts = [end - start for start, end in zip(left, right)]
    total_us = after['sum_us'] - before['sum_us']
    if min(counts) < 0 or total_us < 0:
        raise ValueError('histogram 누적 값 감소')
    samples = sum(counts)
    result = {'samples': samples, 'mean_us': total_us / samples if samples else None, 'overflow': counts[-1]}
    for name, fraction in [('p50_us_upper', .5), ('p95_us_upper', .95), ('p99_us_upper', .99)]:
        cumulative = 0
        result[name] = None
        for index, count in enumerate(counts):
            cumulative += count
            if samples and cumulative >= math.ceil(samples * fraction):
                result[name] = (0 if index == 0 else 2 ** (index - 1)) if index < 32 else None
                break
    return result


async def exercise(state, peers, setup_match):
    """모든 로그인/매치 준비 뒤 동일 구간에 고정 주기로 이동/정지 발행. ACK가 발행량을 줄이지 않는다."""
    from protocol import unpack
    config = state.scenario['performance']
    for start in range(0, len(peers), 4 if state.scenario['setup'] == 'match' else 1):
        group = peers[start:start + (4 if state.scenario['setup'] == 'match' else 1)]
        if state.scenario['setup'] == 'match':
            await setup_match(group, state.scenario)
        else:
            await group[0].login(8910)
    directions = (1, 2, 4, 8)
    rng = random.Random(state.seed)
    chosen = {peer.index: rng.choice(directions) for peer in peers}
    opposite = {1: 2, 2: 1, 4: 8, 8: 4}
    for phase, duration in [('warmup', config['warmup_s']), ('measure', config['duration_s'])]:
        state.phase(phase)
        loop = asyncio.get_running_loop()
        begin, pulse = loop.time(), 0
        next_due = begin
        while next_due < begin + duration:
            await asyncio.sleep(max(0, next_due - loop.time()))
            lag_ms = max(0, loop.time() - next_due) * 1000
            with state.lock:
                state.load_lags[phase].append(lag_ms)
            direction_on = pulse % 2 == 0
            await asyncio.gather(*(peer.send('CS_MOVE', (chosen[peer.index] if (pulse // 2) % 2 == 0 else opposite[chosen[peer.index]]) if direction_on else 0, 0) for peer in peers))
            pulse += 1
            next_due = begin + pulse * config['interval_s']
        await asyncio.sleep(max(0, begin + duration - loop.time()))
    state.phase('verify')
    markers = {peer: peer.sequence for peer in peers}
    await asyncio.gather(*(peer.send('CS_MOVE', 0, 0) for peer in peers))
    drain_error = None
    if 'settle_s' in config:
        from settlement import wait_for_drain
        try:
            await wait_for_drain(state, config['settle_s'])
        except Exception as error:
            drain_error = error

    async def verify(peer):
        async def operation():
            state.peer(peer.index, measure_moves=peer.measure_moves, measure_distance=peer.measure_distance)
            if peer.error:
                raise ConnectionError(peer.error)
            if not peer.measure_moves:
                raise ValueError('측정 구간에 자신의 이동 응답이 없습니다')
            if peer.measure_distance <= .0001:
                raise ValueError('측정 구간에 자신의 위치 변화가 없습니다')
            if peer.port == 8911:
                await peer.expect('SC_MOVE_PLAYER', markers[peer], lambda body: unpack('SC_MOVE_PLAYER', body)[0] == peer.player_id and unpack('SC_MOVE_PLAYER', body)[4] == 0)
            else:
                deadline = asyncio.get_running_loop().time() + peer.timeout
                while True:
                    marker = peer.sequence
                    await asyncio.sleep(.4)
                    if peer.error:
                        raise ConnectionError(peer.error)
                    if peer.moves.get(peer.player_id, (0, None))[0] <= marker:
                        break
                    if asyncio.get_running_loop().time() >= deadline:
                        raise TimeoutError('로비 최종 정지 관찰 timeout')
        try:
            await peer.check('performance-move-stop', operation)
            return True
        except Exception:
            return False

    passed = await asyncio.gather(*(verify(peer) for peer in peers))
    state.phase('cleanup')
    if drain_error:
        raise RuntimeError(str(drain_error)) from drain_error
    if not all(passed):
        raise RuntimeError(f'이동/정지 판정 실패: {passed.count(False)}/{len(peers)}명')
