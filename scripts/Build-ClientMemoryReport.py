"""Publish sanitized memory measurements, calculated payloads, CSV tables and optional charts."""
import argparse
import csv
import hashlib
import json
import statistics
from collections import Counter
from datetime import datetime, timedelta, timezone
from pathlib import Path

MIB = 1048576
METRICS = ['privateBytes', 'workingSetBytes', 'peakWorkingSetBytes',
           'dxgiLocalUsageBytes', 'dxgiNonLocalUsageBytes', 'sampledPeakPrivateBytes']


def describe(values):
    return {'medianBytes': statistics.median(values), 'minBytes': min(values), 'maxBytes': max(values)}


def hero_deltas(snapshots):
    pairs, start = [], None
    for item in snapshots:
        if item['phase'] == 'model_begin:Model/ModularModel.bin':
            start = item
        elif item['phase'] == 'model_loaded:Model/ModularModel.bin':
            if start is None:
                raise ValueError('Missing hero model start')
            pairs.append({key: item[key] - start[key] for key in METRICS[:2] + METRICS[3:5]})
            start = None
    if len(pairs) != 2:
        raise ValueError('Expected two independent ModularModel loads')
    return pairs


def charts(evidence, destination):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib import font_manager
    font = Path('C:/Windows/Fonts/malgun.ttf')
    if font.is_file():
        font_manager.fontManager.addfont(str(font))
        plt.rcParams['font.family'] = font_manager.FontProperties(fname=str(font)).get_name()
    plt.rcParams.update({'axes.unicode_minus': False, 'svg.fonttype': 'none', 'font.size': 11})
    def save_figure(figure, name):
        for extension in ('png', 'svg'):
            path = destination / f'{name}.{extension}'
            figure.savefig(path, dpi=170)
            if extension == 'svg':
                # matplotlib의 XML 들여쓰기 공백을 정리해 Git whitespace 검사도 통과시킨다.
                path.write_text('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines()) + '\n',
                                encoding='utf-8')
    fig, axes = plt.subplots(1, 3, figsize=(13, 5), layout='constrained')
    for ax, key, label in zip(axes, METRICS[:2] + [METRICS[3]],
                             ['프로세스 private commit', '프로세스 Working Set', 'DXGI LOCAL CurrentUsage']):
        stats = evidence['comparison'][key]
        y = [stats[mode]['medianBytes'] / MIB for mode in ('legacy', 'shared')]
        errors = [[(stats[m]['medianBytes'] - stats[m]['minBytes']) / MIB for m in ('legacy', 'shared')],
                  [(stats[m]['maxBytes'] - stats[m]['medianBytes']) / MIB for m in ('legacy', 'shared')]]
        ax.bar(['이전 경로 재현', '현재 공유 경로'], y, color=['#8492a6', '#2879bd'], yerr=errors, capsize=4)
        ax.set_title(label, fontsize=11)
        ax.set_ylabel('MiB')
        ax.set_ylim(0, max(y) * 1.19)
        ax.grid(axis='y', alpha=.2)
        ax.set_axisbelow(True)
        for i, value in enumerate(y):
            ax.text(i, value + max(y) * .025, f'{value:,.1f}', ha='center')
        ax.text(.5, .92, f"감소 {stats['savedPercent']:.1f}%", transform=ax.transAxes, ha='center', color='#175582')
    fig.suptitle('인게임 진입 메모리 · 동일 바이너리의 A/B 경로 비교', fontsize=15)
    fig.supxlabel('Release x64 / RTX 4070 SUPER / 각 3회 중앙값, 오차 막대는 최솟값–최댓값\n각 지표는 별도 측정값이며 서로 합산하지 않음', fontsize=10)
    destination.mkdir(parents=True, exist_ok=True)
    save_figure(fig, 'ingame-memory')
    plt.close(fig)

    hero = next(m for m in evidence['assets']['models'] if m['path'] == 'Model/ModularModel.bin')
    geometry = [hero[m]['cpuArrayBytes'] / MIB for m in ('legacyGeometry', 'sharedGeometry')]
    skin = hero['skin']['cpuArrayBytes'] / MIB
    animation = (hero['animation']['matrixBytes'] + hero['animation']['timeAndRowPointerBytes']) / MIB
    fig, ax = plt.subplots(figsize=(8, 5), layout='constrained')
    labels = ['이전: 모델 1회 로딩', '현재: 캐시가 빈 모델 1회 로딩']
    ax.bar(labels, geometry, label='base geometry CPU 배열', color='#2879bd')
    ax.bar(labels, [skin] * 2, bottom=geometry, label='스키닝 CPU 배열', color='#6bad8b')
    ax.bar(labels, [animation] * 2, bottom=[g + skin for g in geometry],
           label='애니메이션 행렬·시간·행 포인터', color='#d5a044')
    ax.set_ylim(0, 455)
    ax.set_ylabel('계산된 CPU 배열 용량 (MiB)')
    ax.set_title('영웅 ModularModel · 애니메이션 약 334MiB는 그대로 유지')
    for i, value in enumerate(geometry):
        ax.text(i, value + skin + animation + 7, f'{value + skin + animation:.2f} MiB', ha='center')
    ax.legend(loc='upper center', bbox_to_anchor=(.5, .98), fontsize=9)
    fig.supxlabel('텍스처·객체·allocator·GPU/UPLOAD 버퍼 제외 / 화면의 영웅 수로 곱하지 않음', fontsize=9)
    save_figure(fig, 'hero-cpu-payload')
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--summary', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--charts', action='store_true', help='Requires matplotlib; charts are next to evidence')
    args = parser.parse_args()
    source = json.loads(args.summary.read_text(encoding='utf-8-sig'))
    assets = json.loads(args.assets.read_text(encoding='utf-8-sig'))
    expected_loads = Counter({m['path']: m['loadCount'] for m in assets['models']})
    root = Path(__file__).resolve().parents[1]
    runs = []
    for run in source['results']:
        data = run['data']
        snapshots = data['snapshots']
        actual_loads = Counter(s['phase'].split(':', 1)[1] for s in snapshots if s['phase'].startswith('model_loaded:'))
        if not data['ok'] or snapshots[-1]['phase'] != 'ingame_ready' or actual_loads != expected_loads:
            raise ValueError('Incomplete or mismatched measurement scenario')
        runs.append({'run': run['run'], 'mode': run['mode'],
                     'entry': {key: snapshots[-1][key] for key in METRICS[:-1]},
                     'sampledPeakPrivateBytes': run['sampledPeakPrivateBytes'],
                     'heroModelLoadDeltas': hero_deltas(snapshots)})
    modes = {mode: [r for r in runs if r['mode'] == mode] for mode in ('legacy', 'shared')}
    if any(len(items) != source['runsPerMode'] for items in modes.values()):
        raise ValueError('Run count does not match summary')
    if len({r['data']['adapter'] for r in source['results']}) != 1:
        raise ValueError('Adapter changed between runs')
    comparison = {}
    for key in METRICS:
        result = {mode: describe([r.get(key, r['entry'].get(key)) for r in items])
                  for mode, items in modes.items()}
        result['savedBytes'] = result['legacy']['medianBytes'] - result['shared']['medianBytes']
        result['savedPercent'] = result['savedBytes'] / result['legacy']['medianBytes'] * 100
        comparison[key] = result
    files = ['Client/WarOfDimension/ClientMemoryProfile.cpp', 'Client/WarOfDimension/ClientMemoryProfile.h',
             'Client/WarOfDimension/GameFramework.cpp', 'Client/WarOfDimension/GameFramework.h',
             'Client/WarOfDimension/NetworkManager.cpp', 'Client/WarOfDimension/Object.cpp',
             'Client/WarOfDimension/WarOfDimension.cpp', 'Client/WarOfDimension/WarOfDimension.vcxproj',
             'Client/WarOfDimension/WarOfDimension.vcxproj.filters',
             'scripts/Measure-ClientMemory.ps1', 'scripts/Measure-ClientAssets.py']
    timestamp = datetime.fromisoformat(source['measuredAtUtc']).astimezone(timezone(timedelta(hours=9)))
    result = {'schemaVersion': 1, 'kind': 'sanitized_measurement_evidence_and_payload_calculation',
              'measuredAtKst': timestamp.isoformat(), 'configuration': source['configuration'],
              'runsPerMode': source['runsPerMode'], 'samplingIntervalMs': source['samplingIntervalMs'],
              'adapter': source['results'][0]['data']['adapter'],
              'sourceBaseCommit': source['sourceCommit'], 'instrumentedWorkingTree': True,
              'legacyReferenceCommit': 'ed3428e9e092bbb7f83f8d13f0094aa04b7a3361',
              'baselineKind': 'reconstructed_pre_sharing_geometry_path_in_same_binary',
              'scenario': source['results'][0]['data']['scenario'], 'offline': True,
              'frameBuffer': {'width': 1920, 'height': 1080},
              'executableSha256': source['executableSha256'],
              'measurementSourceSha256': {path: hashlib.sha256((root / path).read_bytes()).hexdigest() for path in files},
              'runs': runs, 'comparison': comparison, 'assets': assets}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    with args.output.with_suffix('.csv').open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['metric', 'legacyMedianMiB', 'sharedMedianMiB', 'savedMiB', 'savedPercent',
                         'legacyMinMiB', 'legacyMaxMiB', 'sharedMinMiB', 'sharedMaxMiB'])
        for key, values in comparison.items():
            writer.writerow([key, values['legacy']['medianBytes'] / MIB, values['shared']['medianBytes'] / MIB,
                             values['savedBytes'] / MIB, values['savedPercent'],
                             values['legacy']['minBytes'] / MIB, values['legacy']['maxBytes'] / MIB,
                             values['shared']['minBytes'] / MIB, values['shared']['maxBytes'] / MIB])
    if args.charts:
        charts(result, args.output.parent / 'figures')
    print(f'Memory report: {len(runs)} complete runs, {len(assets["models"])} model assets. {args.output}')


if __name__ == '__main__':
    main()
