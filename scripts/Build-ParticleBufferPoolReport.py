"""고정 파티클 재생의 전용 버퍼/공용 풀 비교. 실전 최대 동시 효과 계측과 구분한다."""
import argparse
import csv
import hashlib
import json
import statistics
from pathlib import Path


def build(summary):
    if summary.get("scenarioKind") != "ParticleReuse":
        raise ValueError("ParticleReuse 시나리오가 필요합니다.")
    modes = {name: [] for name in ("dedicated", "pooled")}
    expected_trace = [4, 19, 8, 0, 18]
    reference = None
    for run in summary["results"]:
        mode = run["mode"]
        if mode not in modes:
            raise ValueError("알 수 없는 비교 모드")
        data = run["data"]
        if not data.get("ok") or not data.get("particleReleaseChecked"):
            raise ValueError("진입/종료 검사가 완료되지 않았습니다.")
        if data.get("scenario") != "fixed_skills_all_categories_synthetic_particle_replay":
            raise ValueError("고정 재생 시나리오 불일치")
        fixture = (data["loadout"], data["heroModels"], data["adapter"])
        if reference is None:
            reference = fixture
        if fixture != reference:
            raise ValueError("선택/모델/adapter 비교 조건 불일치")
        trace = [s for s in data["snapshots"] if s["phase"].startswith("reuse_")]
        if len(trace) != 5:
            raise ValueError("전체 종류 재생 구간 누락")
        for i, sample in enumerate(trace):
            if sample["poolAllocationFailures"] or sample["poolPendingPairs"]:
                raise ValueError("GPU 완료/할당 실패 상태")
            leased = 72 if mode == "dedicated" else expected_trace[i]
            allocated = 72 if mode == "dedicated" else (4 if i == 0 else 19)
            if sample["poolLeasedPairs"] != leased or sample["poolAllocatedPairs"] != allocated:
                raise ValueError("고정 재생 풀 크기 불일치")
        final = data["snapshots"][-1]
        count = 72 if mode == "dedicated" else 19
        if final["phase"] != "ingame_ready" or final["particleCreatedCount"] != count:
            raise ValueError("최종 생성 개수 불일치")
        if final["particleCapacity"] != 300000 or final["particleVertexStride"] != 32:
            raise ValueError("입자 용량/stride 불일치")
        if final["particleLargeBufferPayloadBytes"] != count * 300000 * 32 * 2:
            raise ValueError("payload byte 합계 불일치")
        if final["particleLargeBufferAllocationBytes"] != final["poolAllocationBytes"]:
            raise ValueError("현재 풀 allocation과 생성 계측 불일치")
        if mode == "pooled" and final["poolReuseCount"] != 30:
            raise ValueError("종류 간 재사용 횟수 불일치")
        modes[mode].append(final)
    runs = summary["runsPerMode"]
    if runs < 1 or any(len(items) != runs for items in modes.values()):
        raise ValueError("모드별 반복 횟수 불일치")
    metrics = {}
    for metric in ("privateBytes", "workingSetBytes", "dxgiLocalUsageBytes", "particleLargeBufferPayloadBytes", "particleLargeBufferAllocationBytes"):
        before = statistics.mean(s[metric] for s in modes["dedicated"])
        after = statistics.mean(s[metric] for s in modes["pooled"])
        metrics[metric] = {"dedicatedMeanBytes": before, "pooledMeanBytes": after,
                           "savingBytes": before - after, "savingPercent": (before - after) / before * 100}
    return {"schemaVersion": 1, "kind": "same_binary_all_category_particle_pool_synthetic_replay",
            "measuredAtUtc": summary["measuredAtUtc"], "runsPerMode": runs,
            "executableSha256": summary["executableSha256"], "measurementSourcesSha256": summary["measurementSourcesSha256"],
            "logicalEffectObjects": 72, "dedicatedPairs": 72, "pooledPairs": 19, "pooledFinalLeasedPairs": 18,
            "pooledReuseCount": 30, "activeEffectTrace": expected_trace, "framesPerTraceStep": 2,
            "loadout": reference[0], "heroModels": reference[1], "adapter": reference[2],
            "particleReleaseCheckedEveryRun": True, "runtimePeakMeasured": False,
            "scope": "fixed_synthetic_effect_replay_not_full_gameplay_peak_or_retained_heap_attribution",
            "metrics": metrics}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--verify-current", action="store_true")
    parser.add_argument("--charts", action="store_true")
    args = parser.parse_args()
    summary = json.loads(args.summary.read_text(encoding="utf-8-sig"))
    result = build(summary)
    if args.verify_current:
        root = Path(__file__).resolve().parents[1]
        for name, expected in result["measurementSourcesSha256"].items():
            if hashlib.sha256((root / name).read_bytes()).hexdigest() != expected:
                raise ValueError(f"현재 소스 해시 불일치: {name}")
        binary = root / "artifacts/bin" / summary["configuration"] / "Client/WarOfDimension.exe"
        if hashlib.sha256(binary.read_bytes()).hexdigest() != result["executableSha256"]:
            raise ValueError("현재 실행 파일 해시 불일치")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with args.output.with_suffix(".csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["metric", "dedicatedMeanMiB", "pooledMeanMiB", "savingMiB", "savingPercent"])
        for name, values in result["metrics"].items():
            writer.writerow([name, values["dedicatedMeanBytes"] / 2**20, values["pooledMeanBytes"] / 2**20,
                             values["savingBytes"] / 2**20, values["savingPercent"]])
    if args.charts:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        names = ["particleLargeBufferAllocationBytes", "privateBytes", "dxgiLocalUsageBytes"]
        fig, ax = plt.subplots(figsize=(10, 5))
        before = [result["metrics"][m]["dedicatedMeanBytes"] / 2**20 for m in names]
        after = [result["metrics"][m]["pooledMeanBytes"] / 2**20 for m in names]
        for offset, data, label, color in [(-.18, before, "Dedicated buffers", "#64748b"), (.18, after, "Reusable pool", "#0891b2")]:
            bars = ax.bar([x + offset for x in range(3)], data, .36, label=label, color=color)
            ax.bar_label(bars, fmt="%.1f", padding=3)
        ax.set_xticks(range(3), ["Large GPU allocation", "Private commit", "DXGI LOCAL"])
        ax.set_ylabel("MiB (separate metrics; do not add)")
        ax.set_title("All-category particle replay: 72 dedicated pairs vs 19 pooled pairs\nSynthetic sequence, " + str(result["runsPerMode"]) + " runs per mode; not gameplay peak")
        ax.legend(); ax.set_ylim(0, max(before) * 1.18); fig.tight_layout()
        figures = args.output.parent / "figures"; figures.mkdir(exist_ok=True)
        for suffix in ("png", "svg"):
            fig.savefig(figures / f"particle-buffer-pool-memory.{suffix}", dpi=150)
        plt.close(fig)
    print(json.dumps(result["metrics"]))


if __name__ == "__main__":
    main()
