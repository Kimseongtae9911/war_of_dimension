"""하늘·dissolve DDS와 기존 UI 전후 측정에서 후속 최적화 후보를 계산한다.

읽기 전용 검토다. DDS payload 예상량을 GPU allocation 또는 Private Bytes 절감으로
취급하지 않으며, 에셋 변환·게임 실행·수명 변경은 수행하지 않는다.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics
import struct

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "Client/WarOfDimension"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def mip_bytes(width, height, mips, fmt, faces=1):
    result = []
    for level in range(mips):
        w, h = max(1, width >> level), max(1, height >> level)
        if fmt in ("BC1_UNORM", "BC4_UNORM", "BC7_UNORM"):
            size = ((w + 3) // 4) * ((h + 3) // 4) * (16 if fmt == "BC7_UNORM" else 8)
        else:
            size = w * h * (4 if fmt == "B8G8R8A8_UNORM" else 1)
        result.append(dict(level=level, width=w, height=h, bytesPerFace=size, bytes=size * faces))
    return dict(format=fmt, width=width, height=height, mipLevels=mips, faces=faces,
                payloadBytes=sum(x["bytes"] for x in result), levels=result)


def dds(name):
    path = CLIENT / name
    raw = path.read_bytes()
    if raw[:4] != b"DDS " or struct.unpack_from("<I", raw, 4)[0] != 124:
        raise ValueError(f"DDS header 불일치: {name}")
    height, width = struct.unpack_from("<2I", raw, 12)
    mips = max(1, struct.unpack_from("<I", raw, 28)[0])
    caps2 = struct.unpack_from("<I", raw, 112)[0]
    faces = 6 if caps2 & 0x200 else 1
    if faces == 6 and caps2 & 0xfc00 != 0xfc00:
        raise ValueError("큐브맵의 6면이 모두 필요함")
    fourcc = raw[84:88]
    bits, *masks = struct.unpack_from("<5I", raw, 88)
    if fourcc == b"DXT1":
        fmt = "BC1_UNORM"
    elif fourcc == b"\0\0\0\0" and bits == 32 and masks == [0xff0000, 0xff00, 0xff, 0xff000000]:
        fmt = "B8G8R8A8_UNORM"
    else:
        raise ValueError(f"재검토가 필요한 DDS 형식: {name}")
    result = mip_bytes(width, height, mips, fmt, faces)
    if len(raw) != 128 + result["payloadBytes"]:
        raise ValueError(f"DDS mip/면 payload 불일치: {name}")
    result.update(path=name, sha256=sha(path), fileBytes=len(raw), caps2=caps2,
                  rawMipCount=struct.unpack_from("<I", raw, 28)[0], fourcc=fourcc.hex())
    if fmt == "B8G8R8A8_UNORM":
        alpha = raw[131::4]
        result["opaqueAlphaTexels"] = alpha.count(255)
        result["totalTexels"] = len(alpha)
        result["allAlphaOpaque"] = result["opaqueAlphaTexels"] == len(alpha)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "artifacts/logs/skybox-dissolve-review.json")
    args = parser.parse_args()
    sky, dissolve = dds("SkyBox/Space.dds"), dds("Model/Textures/dissolve.dds")
    assert (sky["width"], sky["height"], sky["mipLevels"], sky["faces"]) == (2048, 2048, 1, 6)
    assert (dissolve["width"], dissolve["height"], dissolve["mipLevels"], dissolve["format"]) == (1024, 1024, 11, "BC1_UNORM")
    candidates = {
        "skyBc7SameResolution": mip_bytes(2048, 2048, 1, "BC7_UNORM", 6),
        "skyBc7FullMips": mip_bytes(2048, 2048, 12, "BC7_UNORM", 6),
        "skyBc1SameResolution": mip_bytes(2048, 2048, 1, "BC1_UNORM", 6),
        "skyBc7HalfResolution": mip_bytes(1024, 1024, 1, "BC7_UNORM", 6),
        "dissolveBc4SameMips": mip_bytes(1024, 1024, 11, "BC4_UNORM"),
        "dissolveR8SameMips": mip_bytes(1024, 1024, 11, "R8_UNORM"),
        "dissolveBc1HalfResolution": mip_bytes(512, 512, 10, "BC1_UNORM"),
    }
    for key, value in candidates.items():
        value["savedPayloadBytes"] = (sky if key.startswith("sky") else dissolve)["payloadBytes"] - value["payloadBytes"]
    evidence_path = ROOT / "docs/portfolio/evidence/ui-bc7-20261005/measurements.json"
    evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    fields = ("privateBytes", "workingSetBytes", "dxgiLocalUsageBytes", "dxgiNonLocalUsageBytes")
    stages = {}
    for phase in ("before", "after"):
        stages[phase] = {}
        for target in ("skybox_ready", "ingame_ui_dissolve_ready"):
            rows = []
            for run in evidence["memory"][phase]["results"]:
                snaps = run["data"]["snapshots"]
                index = next(i for i, value in enumerate(snaps) if value["phase"] == target)
                current, previous = snaps[index], snaps[index - 1]
                rows.append(dict(run=run["run"], precedingPhase=previous["phase"],
                                 deltaBytes={k: current[k] - previous[k] for k in fields}))
            stages[phase][target] = dict(runs=rows, mediansDeltaBytes={
                k: statistics.median(row["deltaBytes"][k] for row in rows) for k in fields})
    source_names = ["GameFramework.cpp", "Object.cpp", "Object.h", "Scene.cpp", "Shader.cpp", "Shader.h",
                    "Common.hlsl", "UI.hlsl", "Skybox.hlsl", "DeferredRender.hlsl", "SharedDdsTexture.cpp", "stdafx.cpp"]
    sources = [f"Client/WarOfDimension/{name}" for name in source_names] + ["scripts/Measure-SkyboxDissolve.py"]
    report = dict(schemaVersion=1, reviewOnly=True, reviewedAtUtc=datetime.now(timezone.utc).isoformat(),
                  units="bytes; MiB = bytes / 1048576",
                  method="Current DDS headers/payload and original cube alpha; historical same-EXE UI 3-run checkpoint deltas; no new runtime allocation measurement or compression applied",
                  current=dict(sky=sky, dissolve=dissolve), candidates=candidates,
                  existingMeasurement=dict(path=evidence_path.relative_to(ROOT).as_posix(), sha256=sha(evidence_path),
                                           measuredAtUtc={p: evidence["memory"][p]["measuredAtUtc"] for p in ("before", "after")},
                                           checkpointDeltas=stages),
                  existingUiActualAllocation={role: {k: evidence["native"][role]["after"]["allocations"][k]
                      for k in ("uniqueTextures", "defaultAllocationBytes", "uploadAllocationBytes")} for role in ("player", "boss")},
                  sourceSha256={name: sha(ROOT / name) for name in sources})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"currentPayloadMiB": {"sky": sky["payloadBytes"] / 1048576,
                     "dissolve": dissolve["payloadBytes"] / 1048576},
                     "skyAllAlphaOpaque": sky["allAlphaOpaque"],
                     "candidateSavedPayloadMiB": {k: v["savedPayloadBytes"] / 1048576 for k, v in candidates.items()}}, ensure_ascii=False))


if __name__ == "__main__":
    main()
