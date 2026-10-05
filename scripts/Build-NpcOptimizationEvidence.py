"""NPC 최적화의 측정·변환·네이티브 검사와 실제 캡처를 공개 근거로 집계한다."""
import argparse
import csv
import hashlib
import json
import shutil
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MIB = 1024 ** 2


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def collect(summary):
    assert summary["configuration"] == "Release" and summary["runsPerMode"] == 3
    assert summary["scenarioKind"] == "ParticleReuse" and len(summary["results"]) == 3
    runs = []
    for run in summary["results"]:
        data = run["data"]
        assert run["mode"] == "pooled" and data["ok"] and data["particleReleaseChecked"]
        assert data["scenario"] == "fixed_skills_all_categories_synthetic_particle_replay"
        ready = [s for s in data["snapshots"] if s["phase"] == "ingame_ready"]
        assert len(ready) == 1
        assert ready[0]["poolAllocatedPairs"] == 19 and ready[0]["poolReuseCount"] == 30
        runs.append(dict(run=run["run"], adapter=data["adapter"], loadout=data["loadout"],
                         ready=ready[0], snapshots=data["snapshots"]))
    return dict(executableSha256=summary["executableSha256"],
                sourcesSha256=summary["measurementSourcesSha256"],
                measuredAtUtc=summary["measuredAtUtc"], runs=runs)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--before", default="artifacts/logs/npc-memory-before/summary-Release.json")
    parser.add_argument("--after", default="artifacts/logs/npc-memory-final/summary-Release.json")
    args = parser.parse_args()
    before, after = collect(read(ROOT / args.before)), collect(read(ROOT / args.after))
    assert sha(ROOT / "artifacts/bin/Release/Client/WarOfDimension.exe") == after["executableSha256"]
    for file, expected in after["sourcesSha256"].items():
        assert sha(ROOT / file) == expected, file
    assert before["runs"][0]["loadout"] == after["runs"][0]["loadout"]
    assert all(r["loadout"] == before["runs"][0]["loadout"] for s in (before, after) for r in s["runs"])
    conversion = read(ROOT / "artifacts/logs/npc-bc7-converted/conversion.json")
    assert conversion["applied"] and len(conversion["textures"]) == 25
    assert sum(t["action"] == "compressed" for t in conversion["textures"]) == 24
    for texture in conversion["textures"]:
        assert sha(ROOT / texture["path"]) == texture["afterSha256"]
        assert texture["mipLevels"] == 1
        if texture["action"] == "preserved":
            assert texture["beforeSha256"] == texture["afterSha256"] and texture["format"] == "R8_UNORM"
        else:
            assert texture["format"] == "BC7_UNORM"
            assert texture["beforePayloadBytes"] == texture["afterPayloadBytes"] * 4
    public = ROOT / "docs/portfolio/evidence/npc-memory-20261005"
    public.mkdir(parents=True, exist_ok=True)
    capture_before = ROOT / "artifacts/logs/npc-bc7-before"
    capture_after = ROOT / "artifacts/logs/npc-bc7-final"
    captures = {"before": read(capture_before / "capture.json"), "after": read(capture_after / "capture.json")}
    assert captures["before"]["views"] == captures["after"]["views"]
    assert captures["after"]["npcTextureResources"] == 25 and captures["after"]["npcUploadsAfterRelease"] == 0
    assert captures["after"]["objectConstantSlots"] == 808 and captures["after"]["objectConstantPages"] == 8
    assert sum(t["beforePayloadBytes"] for t in conversion["textures"]) == captures["before"]["npcTextureAllocationBytes"]
    assert sum(t["afterPayloadBytes"] for t in conversion["textures"]) == captures["after"]["npcTextureAllocationBytes"]
    quality = read(ROOT / "artifacts/logs/npc-quality-final.json")
    assert quality["samePoseCamera"] and len(quality["screenshots"]) == 18 and len(quality["textures"]) == 25
    for key, directory in (("before", capture_before), ("after", capture_after)):
        destination = public / key
        destination.mkdir(exist_ok=True)
        for item in quality["screenshots"]:
            path = directory / item["file"]
            assert sha(path) == item[key + "Sha256"]
            shutil.copyfile(path, destination / item["file"])
        captures[key]["run"] = read(directory / "run.json")
    assert captures["after"]["run"]["executableSha256"] == after["executableSha256"]
    tests = {}
    for configuration in ("Debug", "Release"):
        for name in ("object-constants", "dds-sharing"):
            value = read(ROOT / f"artifacts/logs/test-{name}-{configuration}.json")
            assert value["ok"] and value["gpuErrors"] == 0 and value["liveAfterExit"] == 0
            tests[name + "-" + configuration] = value
    for name in ("mesh-sharing", "hero-selection", "particle-buffer-pool", "particle-selection"):
        value = read(ROOT / f"artifacts/logs/test-{name}-Release.json")
        assert value["ok"]
        tests[name + "-Release"] = value
    diagnostics = {phase: read(ROOT / f"artifacts/logs/npc-bc7-final-debug/gpu-{phase}.json")
                   for phase in ("initialization", "capture-end", "ui-release")}
    assert all(v["errors"] == 0 and v["removedReason"] == 0 for v in diagnostics.values())
    lifetime = read(ROOT / "artifacts/logs/npc-bc7-final/lifetime.json")
    assert lifetime["objectConstantPagesAfterOnDestroy"] == 0
    metrics = {}
    for field in ("privateBytes", "workingSetBytes", "dxgiLocalUsageBytes"):
        a = [r["ready"][field] for r in before["runs"]]
        b = [r["ready"][field] for r in after["runs"]]
        metrics[field] = dict(beforeMedianBytes=statistics.median(a), afterMedianBytes=statistics.median(b),
                              savedBytes=statistics.median(a)-statistics.median(b), beforeRangeBytes=[min(a), max(a)],
                              afterRangeBytes=[min(b), max(b)])
    source_paths = ["scripts/Convert-NpcTextures.ps1", "scripts/Capture-Monsters.ps1",
                    "scripts/Analyze-NpcOptimization.py", "scripts/Build-NpcOptimizationEvidence.py",
                    "Client/WarOfDimension/ObjectConstantArenaTests.cpp", "Client/WarOfDimension/SharedDdsTextureTests.cpp",
                    "Client/WarOfDimension/WarOfDimension.vcxproj", "Client/WarOfDimension/WarOfDimension.vcxproj.filters"]
    result = dict(schemaVersion=1, kind="npc_memory_optimization", unit="bytes",
                  comparison="DDS 공유를 양쪽에 유지한 별도 바이너리 전후 각각 3회 순차 측정; 같은 fixture",
                  metrics=metrics, before=before, after=after, conversion=conversion, captures=captures,
                  objectConstants=dict(logicalSlots=808, frames=2, stride=256, pageBytes=65536,
                                       beforeAllocationBytes=808*65536, afterAllocationBytes=8*65536),
                  tests=tests, debugGpu=diagnostics, lifetime=lifetime,
                  additionalSourcesSha256={p: sha(ROOT / p) for p in source_paths},
                  limitations=["고정 포즈 모델 비교; 실제 멀티플레이 전투 전체 검증 아님",
                               "Private Bytes·Working Set·DXGI LOCAL은 합산하지 않음",
                               "BC7 손실 압축; 모든 조명·움직임·거리에서 동일 화질을 증명하지 않음",
                               "실제 장치 OOM·운영 DB·블록체인 미검증"])
    dump(public / "measurements.json", result)
    dump(public / "quality.json", quality)
    with (public / "memory.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["mode", "run", "privateBytes", "workingSetBytes", "dxgiLocalUsageBytes"])
        for mode, data in (("before", before), ("after", after)):
            for run in data["runs"]:
                writer.writerow([mode, run["run"], *(run["ready"][f] for f in metrics)])
    for field, value in metrics.items():
        print(field, *(round(value[k]/MIB, 4) for k in ("beforeMedianBytes", "afterMedianBytes", "savedBytes")))
    print("현재 소스/EXE/25 DDS·카메라·36 PNG 해시·native 종료·측정 계산 PASS")


if __name__ == "__main__":
    main()
