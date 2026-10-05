"""Summarize sequential loading phase deltas; these are not retained heap ownership measurements."""
import argparse
import csv
import json
import statistics
from datetime import datetime, timedelta, timezone
from pathlib import Path

MIB = 1048576
METRICS = ['privateBytes', 'workingSetBytes', 'dxgiLocalUsageBytes', 'dxgiNonLocalUsageBytes']
STAGES = [
    ('startup', '기동·Title·공통 자원', None, 'scene_setup_ready'),
    ('skybox', '하늘 cubemap', 'scene_setup_ready', 'skybox_ready'),
    ('map', '맵 전체', 'skybox_ready', 'map_ready'),
    ('enemies', '미니언·몬스터·보스', 'map_ready', 'boss_ready'),
    ('skills', '스킬 모델·객체·billboard', 'boss_ready', 'skill_objects_ready'),
    ('particle_setup', '파티클 텍스처·공통 설정', 'skill_objects_ready', 'particle_shared_setup_ready'),
    ('particles', '파티클 객체·대형 버퍼', 'particle_shared_setup_ready', 'ingame_scene_objects'),
    ('heroes', '영웅 모델 2회·컨트롤러 4개', 'ingame_scene_objects', 'local_hero_created'),
    ('ingame_ui', '인게임 UI·dissolve', 'local_hero_created', 'ingame_ui_dissolve_ready'),
    ('submission', 'GPU 제출·대기·upload 해제', 'ingame_ui_dissolve_ready', 'ingame_ready'),
]


def stats(values):
    return {'meanBytes': statistics.mean(values), 'medianBytes': statistics.median(values),
            'minBytes': min(values), 'maxBytes': max(values)}


def summarize(summary, historical, mode='shared'):
    allowed_modes = ('legacy', 'shared', 'full', 'selected')
    if mode not in allowed_modes or any(item['mode'] not in allowed_modes for item in summary['results']):
        raise ValueError('Unknown measurement mode')
    runs = []
    for item in summary['results']:
        if item['mode'] != mode:
            continue
        profile = item['data']
        if not profile['ok'] or not profile['snapshots'] or profile['snapshots'][-1]['phase'] != 'ingame_ready':
            raise ValueError('Expected complete measurement runs')
        snapshots = {s['phase']: s for s in profile['snapshots'] if not s['phase'].startswith('model_')}
        phases = list(snapshots)
        if any(end not in snapshots or (begin and phases.index(begin) >= phases.index(end))
               for _, _, begin, end in STAGES):
            raise ValueError('Missing or unordered loading checkpoint')
        stage_values = {key: {metric: snapshots[end][metric] - (snapshots[begin][metric] if begin else 0)
                             for metric in METRICS} for key, _, begin, end in STAGES}
        for metric in METRICS:
            if sum(stage[metric] for stage in stage_values.values()) != snapshots['ingame_ready'][metric]:
                raise ValueError('Sequential deltas do not reconcile to entry total')
        particle_phases = ['particle_shared_setup_ready', 'particle_skill_pool_ready',
                           'particle_selected_skills_ready', 'particle_environment_ready']
        particle_groups = {key: snapshots[end]['particleCreatedCount'] - snapshots[begin]['particleCreatedCount']
                           for key, begin, end in zip(['skillPool', 'selectedSkills', 'environment'],
                                                      particle_phases, particle_phases[1:])}
        final = snapshots['ingame_ready']
        particle_values = {key: final[key] for key in final if key.startswith('particle')}
        count, capacity, stride = (final[key] for key in ['particleCreatedCount', 'particleCapacity', 'particleVertexStride'])
        if sum(particle_groups.values()) != count or final['particleLargeBufferPayloadBytes'] != count * capacity * stride * 2:
            raise ValueError('Particle counter and actual buffer widths do not match')
        runs.append({'run': item['run'], 'entry': {metric: final[metric] for metric in METRICS},
                     'stages': stage_values, 'particleGroups': particle_groups,
                     'particleAllocations': particle_values, 'checkpoints': snapshots})
    if len(runs) != summary['runsPerMode'] or not runs or len({r['run'] for r in runs}) != len(runs):
        raise ValueError('Run count mismatch')
    if any(r['particleAllocations'] != runs[0]['particleAllocations'] or
           r['particleGroups'] != runs[0]['particleGroups'] for r in runs):
        raise ValueError('Particle scenario changed between runs')
    totals = {metric: stats([r['entry'][metric] for r in runs]) for metric in METRICS}
    stages = []
    for key, label, begin, end in STAGES:
        values = {metric: stats([r['stages'][key][metric] for r in runs]) for metric in METRICS}
        stages.append({'key': key, 'label': label, 'begin': begin, 'end': end, 'metrics': values,
                       'privateCommitSharePercent': values['privateBytes']['meanBytes'] / totals['privateBytes']['meanBytes'] * 100})
    timestamp = datetime.fromisoformat(summary['measuredAtUtc']).astimezone(timezone(timedelta(hours=9)))
    result = {'schemaVersion': 1, 'kind': 'phase_net_growth_attribution_not_retained_heap_ownership',
              'measuredAtKst': timestamp.isoformat(), 'configuration': summary['configuration'],
              'sourceBaseCommit': summary['sourceCommit'], 'instrumentedWorkingTree': True,
              'executableSha256': summary['executableSha256'],
              'measurementSourcesSha256': summary['measurementSourcesSha256'],
              'scenario': next(item['data']['scenario'] for item in summary['results'] if item['mode'] == mode),
              'adapter': next(item['data']['adapter'] for item in summary['results'] if item['mode'] == mode), 'runs': runs,
              'totals': totals, 'stages': stages, 'particleGroups': runs[0]['particleGroups'],
              'particleAllocations': runs[0]['particleAllocations']}
    if historical:
        comparison = {}
        for mode in ['legacy', 'shared']:
            deltas = []
            for run in historical['results']:
                if run['mode'] != mode:
                    continue
                points = {p['phase']: p for p in run['data']['snapshots']}
                begin, end = points['model_begin:Model/Plane1.bin'], points['model_begin:Model/FreeLichPBR.bin']
                deltas.append({metric: end[metric] - begin[metric] for metric in METRICS})
            comparison[mode] = {metric: stats([d[metric] for d in deltas]) for metric in METRICS}
        result['historicalMapInterval'] = {'scope': 'Plane1 model begin to next minion model begin, includes CMap CB creation',
                                           'referenceEvidence': 'client-memory-20261005.json', 'metrics': comparison}
    return result


def compare(summary, hero=False, particles=False):
    scenarios = {(item['data']['scenario'], item['data']['adapter']) for item in summary['results']}
    if len(scenarios) != 1:
        raise ValueError('Different scenarios or adapters in comparison')
    expected = {'full': 'full_skill_particle_pool', 'selected': 'selected_skill_particle_pool'} if particles else {'full': 'full_hero_parts', 'selected': 'selected_hero_parts'} if hero else {
        'legacy': 'reconstructed_pre_sharing_geometry_path', 'shared': 'current_shared_geometry_path'}
    before_mode, after_mode = list(expected)
    if any(item['mode'] not in expected or item['data']['baselineKind'] != expected[item['mode']]
           for item in summary['results']):
        raise ValueError('Unexpected geometry baseline')
    modes = {mode: summarize(summary, None, mode) for mode in expected}
    if not particles and any(modes[before_mode][key] != modes[after_mode][key] for key in ('particleGroups', 'particleAllocations')):
        raise ValueError('Particle scenario differs between geometry modes')
    if particles:
        if next(iter(scenarios))[0] != 'fresh_process_title_to_ingame_fixed_skills_ogre':
            raise ValueError('Unexpected particle measurement scenario')
        fixture = {'jobs': [0, 1, 2, 4], 'skills': [[48, 53, 54, 58], [60, 65, 69, 70], [72, 79, 81, 82], [24, 27, 29, 30]]}
        if any(item['data'].get('loadout') != fixture or not item['data'].get('particleReleaseChecked') for item in summary['results']):
            raise ValueError('Different skill fixture or incomplete particle release audit')
        for mode, pool in [('full', 100), ('selected', 45)]:
            modes[mode]['loadout'] = fixture
            modes[mode]['particleReleaseChecked'] = True
            group = modes[mode]['particleGroups']
            if group != {'skillPool': pool, 'selectedSkills': 8, 'environment': 19}:
                raise ValueError('Unexpected particle pool/slot/environment counts')
        if any(modes['full']['particleAllocations'][key] != modes['selected']['particleAllocations'][key]
               for key in ('particleCapacity', 'particleVertexStride')):
            raise ValueError('Particle capacity or stride changed')
    if hero or particles:
        for mode in expected:
            model_counts = [item['data']['heroModels'] for item in summary['results'] if item['mode'] == mode]
            if any(count != model_counts[0] for count in model_counts) or model_counts[0]['loads'] != 2:
                raise ValueError('Different hero model scenario between runs')
            modes[mode]['heroModels'] = model_counts[0]

    def differences(before, after):
        result = {}
        for metric in METRICS:
            old_mean, new_mean = before[metric]['meanBytes'], after[metric]['meanBytes']
            result[metric] = {'before': before[metric], 'after': after[metric],
                              'savedMeanBytes': old_mean - new_mean,
                              'savedPercent': (old_mean - new_mean) / old_mean * 100 if old_mean > 0 else None}
        return result

    stages = []
    for before, after in zip(modes[before_mode]['stages'], modes[after_mode]['stages']):
        if before['key'] != after['key']:
            raise ValueError('Different stage definitions')
        stages.append({'key': before['key'], 'label': before['label'],
                       'metrics': differences(before['metrics'], after['metrics'])})
    return {'schemaVersion': 1, 'kind': 'same_binary_full_vs_selected_skill_particle_stage_comparison' if particles else 'same_binary_full_vs_selected_hero_parts_stage_comparison' if hero else 'same_binary_pre_sharing_geometry_reconstruction_stage_comparison',
            'referenceCommit': summary['sourceCommit'] if (hero or particles) else 'ed3428e9e092bbb7f83f8d13f0094aa04b7a3361',
            'scope': 'phase_net_growth_attribution_not_retained_heap_ownership',
            'runsPerMode': summary['runsPerMode'], 'modes': modes, 'stages': stages,
            'totals': differences(modes[before_mode]['totals'], modes[after_mode]['totals'])}


def comparison_charts(evidence, destination):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib import font_manager
    font = Path('C:/Windows/Fonts/malgun.ttf')
    if font.is_file():
        font_manager.fontManager.addfont(str(font))
        plt.rcParams['font.family'] = font_manager.FontProperties(fname=str(font)).get_name()
    plt.rcParams.update({'axes.unicode_minus': False, 'svg.fonttype': 'none', 'font.size': 10})
    stages = sorted(evidence['stages'], key=lambda row: row['metrics']['privateBytes']['before']['meanBytes'], reverse=True)
    fig, axes = plt.subplots(1, 3, figsize=(17, 9), sharey=True, layout='constrained')
    hero = 'hero_parts' in evidence['kind']
    particles = 'skill_particle' in evidence['kind']
    before_label, after_label = ('전체 스킬 풀', '선택 스킬 풀') if particles else ('전체 파츠', '확정 선택 파츠') if hero else ('공유 전 경로 재현', '메시 공유 후')
    for ax, metric, title in zip(axes, METRICS[:3], ['Private commit', 'Working Set', 'DXGI LOCAL']):
        all_values = []
        for side, offset, color, label in [('before', -.19, '#a67958', before_label),
                                           ('after', .19, '#447ca3', after_label)]:
            values = [row['metrics'][metric][side]['meanBytes'] / MIB for row in stages]
            all_values.extend(values)
            positions = [i + offset for i in range(len(stages))]
            ax.barh(positions, values, height=.36, color=color, label=label)
            for position, value in zip(positions, values):
                ax.text(value + (15 if value >= 0 else -15), position, f'{value:,.1f}', va='center',
                        ha='left' if value >= 0 else 'right', fontsize=8)
        ax.set_yticks(range(len(stages)), [row['label'] for row in stages])
        ax.set_xlim(min(0, min(all_values)) * 1.6 - 90, max(all_values) * 1.2 + 60)
        ax.set_title(title)
        ax.set_xlabel('구간별 순증가량 (MiB)')
        ax.grid(axis='x', alpha=.2)
        ax.set_axisbelow(True)
    axes[0].invert_yaxis()
    axes[0].legend(loc='lower right', fontsize=9)
    fig.suptitle(('선택 스킬 파티클 풀 생성 전후' if particles else '영웅 선택 파츠 로딩 전후' if hero else '메시 공유 전후') + '의 구간별 메모리 · 동일 바이너리 각 3회 평균', fontsize=15)
    fig.supxlabel(('메시 공유·선택 외형은 양쪽 활성 / 네 참가자의 동일 스킬 fixture' if particles else '메시 공유는 양쪽에서 활성 / 남녀·서로 다른 외형의 동일 fixture' if hero else '과거 실행 파일 자체 측정이 아닌 이전 geometry 경로 재현') + ' / 최종 소유권 분석이 아니며 지표를 합산하지 않음', fontsize=10)
    destination.mkdir(parents=True, exist_ok=True)
    for extension in ('png', 'svg'):
        path = destination / f'{"particle-selected-skills-memory" if particles else "hero-selected-parts-memory" if hero else "client-memory-stage-comparison"}.{extension}'
        fig.savefig(path, dpi=170)
        if extension == 'svg':
            path.write_text('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines()) + '\n', encoding='utf-8')
    plt.close(fig)


def charts(evidence, destination):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib import font_manager
    font = Path('C:/Windows/Fonts/malgun.ttf')
    if font.is_file():
        font_manager.fontManager.addfont(str(font))
        plt.rcParams['font.family'] = font_manager.FontProperties(fname=str(font)).get_name()
    plt.rcParams.update({'axes.unicode_minus': False, 'svg.fonttype': 'none', 'font.size': 10})
    stages = sorted(evidence['stages'], key=lambda row: row['metrics']['privateBytes']['meanBytes'], reverse=True)
    labels = [row['label'] for row in stages]
    fig, axes = plt.subplots(1, 3, figsize=(15, 7), sharey=True, layout='constrained')
    colors = ['#b66535' if row['key'] == 'particles' else '#3089a0' if row['key'] == 'map' else '#507dab' for row in stages]
    for ax, metric, title in zip(axes, METRICS[:3], ['Private commit', 'Working Set', 'DXGI LOCAL']):
        values = [row['metrics'][metric]['meanBytes'] / MIB for row in stages]
        ax.barh(labels, values, color=colors)
        for i, value in enumerate(values):
            ax.text(max(value, 0) + 20, i, f'{value:,.1f}', va='center', fontsize=9)
        ax.set_xlim(min(0, min(values)) - 40, max(values) * 1.18 + 60)
        ax.set_title(title)
        ax.set_xlabel('구간별 순증가량 (MiB)')
        ax.grid(axis='x', alpha=.2)
        ax.set_axisbelow(True)
    axes[0].invert_yaxis()
    fig.suptitle('클라이언트 진입 메모리 증가 원인 · 현재 공유 경로 3회 평균', fontsize=15)
    fig.supxlabel('생성 구간의 증가량이며 최종 heap 소유권 분석이 아님 / 서로 다른 지표는 합산하지 않음', fontsize=10)
    destination.mkdir(parents=True, exist_ok=True)
    for extension in ['png', 'svg']:
        path = destination / f'client-memory-breakdown.{extension}'
        fig.savefig(path, dpi=170)
        if extension == 'svg':
            path.write_text('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines()) + '\n', encoding='utf-8')
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--summary', type=Path, required=True)
    parser.add_argument('--historical-summary', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--charts', action='store_true')
    parser.add_argument('--compare', action='store_true', help='Compare legacy/shared runs from the same summary')
    parser.add_argument('--compare-hero', action='store_true', help='Compare full/selected hero parts from the same summary')
    parser.add_argument('--compare-particles', action='store_true', help='Compare full/selected skill particle pools')
    args = parser.parse_args()
    source = json.loads(args.summary.read_text(encoding='utf-8-sig'))
    old = json.loads(args.historical_summary.read_text(encoding='utf-8-sig')) if args.historical_summary else None
    if sum((args.compare, args.compare_hero, args.compare_particles)) > 1:
        parser.error('Choose one comparison mode')
    if (args.compare or args.compare_hero or args.compare_particles) and old:
        parser.error('--compare uses one same-binary summary; omit --historical-summary')
    if args.compare or args.compare_hero or args.compare_particles:
        evidence = compare(source, hero=args.compare_hero, particles=args.compare_particles)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        with args.output.with_suffix('.csv').open('w', encoding='utf-8-sig', newline='') as stream:
            writer = csv.writer(stream)
            columns = [f'{metric}_{field}' for metric in METRICS for field in ('beforeMeanMiB', 'afterMeanMiB', 'savedMeanMiB')]
            writer.writerow(['stage', 'label', *columns])
            for row in [*evidence['stages'], {'key': 'total', 'label': '최종 평균', 'metrics': evidence['totals']}]:
                writer.writerow([row['key'], row['label'], *[value for metric in METRICS for value in (
                    row['metrics'][metric]['before']['meanBytes'] / MIB,
                    row['metrics'][metric]['after']['meanBytes'] / MIB,
                    row['metrics'][metric]['savedMeanBytes'] / MIB)]])
                print(row['key'], {metric: {side: round(row['metrics'][metric][side]['meanBytes'] / MIB, 2)
                                           for side in ('before', 'after')} for metric in METRICS[:3]})
        if args.charts:
            comparison_charts(evidence, args.output.parent / 'figures')
        return
    evidence = summarize(source, old)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    with args.output.with_suffix('.csv').open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['stage', 'label', *[m + 'MeanMiB' for m in METRICS], 'privateCommitSharePercent'])
        for stage in evidence['stages']:
            writer.writerow([stage['key'], stage['label'], *[stage['metrics'][m]['meanBytes'] / MIB for m in METRICS],
                             stage['privateCommitSharePercent']])
    if args.charts:
        charts(evidence, args.output.parent / 'figures')
    for stage in evidence['stages']:
        print(stage['key'], round(stage['metrics']['privateBytes']['meanBytes'] / MIB, 2),
              round(stage['privateCommitSharePercent'], 2))
    print('Entry mean MiB:', {k: round(v['meanBytes'] / MIB, 2) for k, v in evidence['totals'].items()})
    print('Particles:', evidence['particleGroups'], evidence['particleAllocations'])


if __name__ == '__main__':
    main()
