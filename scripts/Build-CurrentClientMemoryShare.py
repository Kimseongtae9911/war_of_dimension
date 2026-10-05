"""공용 풀 적용 후 로딩·고정 재생 구간의 순증가 비중. retained heap 소유량과 구분한다."""
import argparse
import csv
import hashlib
import json
import statistics
from pathlib import Path

from importlib.util import module_from_spec, spec_from_file_location


def load_module(filename):
    spec = spec_from_file_location(filename, Path(__file__).with_name(filename + '.py'))
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


METRICS = ['privateBytes', 'workingSetBytes', 'dxgiLocalUsageBytes', 'dxgiNonLocalUsageBytes']
STAGES = [
    ('startup', '기동·Title·공통 자원', None, 'scene_setup_ready'),
    ('skybox', '하늘 cubemap', 'scene_setup_ready', 'skybox_ready'),
    ('map', '맵 전체', 'skybox_ready', 'map_ready'),
    ('enemies', '미니언·몬스터·보스', 'map_ready', 'boss_ready'),
    ('skills', '스킬 모델·객체·billboard', 'boss_ready', 'skill_objects_ready'),
    ('particle_setup', '파티클 텍스처·공통 설정', 'skill_objects_ready', 'particle_shared_setup_ready'),
    ('particle_objects', '파티클 객체·소형 버퍼', 'particle_shared_setup_ready', 'ingame_scene_objects'),
    ('heroes', '영웅 모델·컨트롤러', 'ingame_scene_objects', 'local_hero_created'),
    ('ingame_ui', '인게임 UI·dissolve', 'local_hero_created', 'ingame_ui_dissolve_ready'),
    ('submission', 'GPU 제출·대기', 'ingame_ui_dissolve_ready', 'ingame_before_particle_use'),
    ('replay', '파티클 고정 재생', 'ingame_before_particle_use', 'reuse_mixed_eighteen'),
    ('finalization', '재생 후 진입 완료', 'reuse_mixed_eighteen', 'ingame_ready'),
]


def build(summary):
    load_module('Build-ParticleBufferPoolReport').build(summary)
    runs = []
    for run in summary['results']:
        if run['mode'] != 'pooled':
            continue
        samples = [s for s in run['data']['snapshots'] if not s['phase'].startswith('model_')]
        points = {s['phase']: s for s in samples}
        order = list(points)
        if len(points) != len(samples):
            raise ValueError('중복 checkpoint')
        values = {}
        for key, _, begin, end in STAGES:
            if end not in points or (begin and (begin not in points or order.index(begin) >= order.index(end))):
                raise ValueError('누락되거나 순서가 잘못된 checkpoint')
            values[key] = {m: points[end][m] - (points[begin][m] if begin else 0) for m in METRICS}
        for m in METRICS:
            if sum(v[m] for v in values.values()) != points['ingame_ready'][m]:
                raise ValueError('구간 합계와 최종 사용량 불일치')
        runs.append({'run': run['run'], 'stages': values,
                     'beforeReplay': {m: points['ingame_before_particle_use'][m] for m in METRICS},
                     'final': {m: points['ingame_ready'][m] for m in METRICS}})
    if len({r['run'] for r in runs}) != summary['runsPerMode']:
        raise ValueError('반복 실행 번호 불일치')
    totals = {m: statistics.mean(r['final'][m] for r in runs) for m in METRICS}
    stages = []
    for key, label, begin, end in STAGES:
        means = {m: statistics.mean(r['stages'][key][m] for r in runs) for m in METRICS}
        stages.append({'key': key, 'label': label, 'begin': begin, 'end': end,
                       'meanBytes': means, 'privateCommitSharePercent': means['privateBytes'] / totals['privateBytes'] * 100})
    return {'schemaVersion': 1, 'scope': 'phase_net_growth_not_retained_heap_ownership',
            'sourceEvidence': 'particle-buffer-pool-runs-20261005.json',
            'sourceEvidenceMeasuredAtUtc': summary['measuredAtUtc'],
            'executableSha256': summary['executableSha256'], 'runsPerMode': summary['runsPerMode'],
            'scenario': 'pooled_fixed_synthetic_replay_not_full_gameplay_peak',
            'totalsMeanBytes': totals,
            'beforeReplayMeanBytes': {m: statistics.mean(r['beforeReplay'][m] for r in runs) for m in METRICS},
            'stages': stages, 'runs': runs}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--summary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--charts', action='store_true')
    args = parser.parse_args()
    result = build(json.loads(args.summary.read_text(encoding='utf-8-sig')))
    result['sourceEvidenceSha256'] = hashlib.sha256(args.summary.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    with args.output.with_suffix('.csv').open('w', encoding='utf-8', newline='') as out:
        writer = csv.writer(out)
        writer.writerow(['stage', 'privateMiB', 'privatePercent', 'workingSetMiB', 'dxgiLocalMiB'])
        for s in result['stages']:
            m = s['meanBytes']
            writer.writerow([s['label'], m['privateBytes'] / 2**20, s['privateCommitSharePercent'], m['workingSetBytes'] / 2**20, m['dxgiLocalUsageBytes'] / 2**20])
    if args.charts:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        plt.rcParams['font.family'] = 'Malgun Gothic'
        plt.rcParams['axes.unicode_minus'] = False
        stages = sorted(result['stages'], key=lambda s: -s['meanBytes']['privateBytes'])
        shown = [s for s in stages if s['privateCommitSharePercent'] >= .1]
        minor = sum(s['meanBytes']['privateBytes'] for s in stages if s not in shown)
        if minor:
            shown.append({'label': '제출·대기·마무리', 'meanBytes': {'privateBytes': minor}, 'privateCommitSharePercent': minor / result['totalsMeanBytes']['privateBytes'] * 100})
        fig, ax = plt.subplots(figsize=(11, 6.6))
        values = [s['privateCommitSharePercent'] for s in shown]
        bars = ax.barh([s['label'] for s in shown], values, color='#247b91', height=.65)
        ax.invert_yaxis()
        ax.bar_label(bars, labels=[f"{s['privateCommitSharePercent']:.2f}%  |  {s['meanBytes']['privateBytes']/2**20:,.2f} MiB" for s in shown], padding=7, fontsize=10)
        ax.set_xlim(0, max(values) * 1.42)
        ax.set_xlabel('최종 Private commit 대비 구간별 순증가 비율 (%)')
        ax.set_title(f"공용 파티클 풀 적용 후 메모리 구성\nPrivate commit {result['totalsMeanBytes']['privateBytes']/2**20:,.2f} MiB · Release 3회 평균", pad=17)
        for side in ['top', 'right']:
            ax.spines[side].set_visible(False)
        fig.text(.5, .015, '고정 효과 재생 기준 · 실전 최대치 및 자원별 최종 소유 메모리 측정이 아님', ha='center', fontsize=10)
        fig.tight_layout(rect=(0, .04, 1, 1))
        destination = args.output.parent / 'figures'
        destination.mkdir(exist_ok=True)
        for ext in ['png', 'svg']:
            path = destination / ('client-memory-current-share.' + ext)
            fig.savefig(path, dpi=160)
            if ext == 'svg':
                path.write_text('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines()) + '\n', encoding='utf-8')
        plt.close(fig)
    print(json.dumps({'totalsMeanBytes': result['totalsMeanBytes'], 'beforeReplayMeanBytes': result['beforeReplayMeanBytes']}, ensure_ascii=False))


if __name__ == '__main__':
    main()
