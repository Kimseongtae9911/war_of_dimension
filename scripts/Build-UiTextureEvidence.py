"""UI BC7 변환·전후 GPU 화면·역할별 할당량·3회 메모리 측정의 공개 근거를 만든다."""
import argparse
import csv
import hashlib
import json
import math
import shutil
import statistics
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def metrics(before, after, mask=None):
    difference = before.astype(np.float32) - after.astype(np.float32)
    if mask is not None:
        difference = difference[mask]
    difference = difference.reshape(-1, before.shape[-1])
    mse = np.mean(difference.astype(np.float64) ** 2, axis=0)
    return dict(pixels=len(difference), changedPixels=int(np.count_nonzero(np.any(difference != 0, axis=1))),
                mse=mse.tolist(), maxAbsoluteError=np.max(np.abs(difference), axis=0).tolist(),
                psnrDb=[None if v == 0 else 10 * math.log10(255**2 / v) for v in mse])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "docs/portfolio/evidence/ui-bc7-20261005")
    args = parser.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    converted = ROOT / "artifacts/logs/ui-bc7-converted"
    original = ROOT / "artifacts/logs/ui-bc7-before/Client/WarOfDimension"
    conversion = load(converted / "conversion.json")
    assert conversion["applied"] and len(conversion["textures"]) == 21
    texture_quality = []
    for t in conversion["textures"]:
        assert sha(original / t["path"]) == t["beforeSha256"]
        assert sha(ROOT / "Client/WarOfDimension" / t["path"]) == t["afterSha256"]
        a = np.asarray(Image.open(original / t["path"]).convert("RGBA"))
        b = Image.open(ROOT / "Client/WarOfDimension" / t["path"]).convert("RGBA")
        b = np.asarray(b.crop((t["offsetX"], t["offsetY"], t["offsetX"]+t["width"], t["offsetY"]+t["height"])))
        assert a.shape == b.shape
        padded = Image.open(converted / "padded" / t["path"]).convert("RGBA")
        assert np.array_equal(a, np.asarray(padded.crop((t["offsetX"], t["offsetY"], t["offsetX"]+t["width"], t["offsetY"]+t["height"]))))
        item = dict(path=t["path"], rgba=metrics(a, b), composites=[])
        for bg in (20, 191):
            aa, ba = a[:, :, 3:4].astype(np.float32)/255, b[:, :, 3:4].astype(np.float32)/255
            item["composites"].append(dict(background=bg, **metrics(a[:, :, :3]*aa+bg*(1-aa), b[:, :, :3]*ba+bg*(1-ba))))
        item["transparentPixelsBecameVisible"] = int(np.count_nonzero((a[:, :, 3] == 0) & (b[:, :, 3] > 2)))
        texture_quality.append(item)
    native = {}
    screenshots = []
    release_binary = None
    for role in ("player", "boss"):
        native[role] = {}
        before = ROOT / f"artifacts/logs/ui-capture-before-final/{role}"
        after = ROOT / f"artifacts/logs/ui-capture-after/{role}"
        ca, cb = load(before/"capture.json"), load(after/"capture.json")
        assert ca == cb and len(ca["views"]) == 16
        assert load(before/"run.json")["executableSha256"] == load(after/"run.json")["executableSha256"]
        release_binary = load(after/"run.json")["executableSha256"]
        for phase, folder in (("before", before), ("after", after), ("debug", ROOT/f"artifacts/logs/ui-capture-debug/{role}")):
            alloc, lifetime = load(folder/"allocations.json"), load(folder/"ui-lifetime.json")
            assert alloc["uniqueTextures"] == 20 and alloc["layoutFailureCases"] == 9
            assert lifetime == dict(checkedResources=40, resourcesRetainedAfterOnDestroy=0)
            diagnostics = {}
            for step in ("initialization", "ui-capture-end", "ui-release"):
                p = folder/f"gpu-{step}.json"
                if p.exists():
                    diagnostics[step] = load(p)
                    assert diagnostics[step]["errors"] == 0 and diagnostics[step]["removedReason"] == 0
            native[role][phase] = dict(allocations=alloc, lifetime=lifetime, run=load(folder/"run.json"), gpuDiagnostics=diagnostics)
        for view in ca["views"]:
            aa = np.asarray(Image.open(before/view["file"]).convert("RGB"))
            bb = np.asarray(Image.open(after/view["file"]).convert("RGB"))
            assert aa.shape == bb.shape == (1080, 1920, 3)
            mask = np.any(aa != aa[0,0], axis=2) | np.any(bb != bb[0,0], axis=2)
            image_paths = {}
            for phase, folder in (("before", before), ("after", after)):
                dest = out/phase/role/view["file"]; dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(folder/view["file"], dest)
                image_paths[phase] = dict(path=dest.relative_to(out).as_posix(), sha256=sha(dest))
            screenshots.append(dict(role=role, file=view["file"], images=image_paths,
                                    whole=metrics(aa, bb), foreground=metrics(aa, bb, mask)))
    memory = {}
    fields = ("privateBytes", "workingSetBytes", "dxgiLocalUsageBytes", "dxgiNonLocalUsageBytes")
    rows = []
    for phase, folder in (("before", "ui-memory-before-final"), ("after", "ui-memory-after")):
        summary = load(ROOT/f"artifacts/logs/{folder}/summary-Release.json")
        assert summary["executableSha256"] == release_binary and summary["runsPerMode"] == 3
        assert len(summary["results"]) == 3 and summary["scenarioKind"] == "ParticleReuse"
        results = []
        for run in summary["results"]:
            d = run["data"]; snap = d["snapshots"][-1]
            assert d["ok"] and snap["phase"] == "ingame_ready"
            assert snap["poolAllocatedPairs"] == 19 and snap["poolReuseCount"] == 30
            assert d["loadout"] == dict(jobs=[0,1,2,4], skills=[[48,53,54,58],[60,65,69,70],[72,79,81,82],[24,27,29,30]])
            results.append(dict(run=run["run"], data=d))
            rows.append(dict(phase=phase, run=run["run"], **{k:snap[k] for k in fields}))
        medians = {k:statistics.median(r["data"]["snapshots"][-1][k] for r in results) for k in fields}
        memory[phase] = dict(executableSha256=summary["executableSha256"], measuredAtUtc=summary["measuredAtUtc"],
                             mediansBytes=medians, results=results)
    source_paths = ["Client/WarOfDimension/" + name for name in
                    ("stdafx.cpp", "stdafx.h", "Object.cpp", "Object.h", "Mesh.cpp", "Mesh.h", "Common.hlsl", "UI.hlsl",
                     "UiTextureLayout.h", "UiTextureCapture.cpp", "GameFramework.h", "ClientMemoryProfile.cpp", "ClientMemoryProfile.h",
                     "WarOfDimension.cpp", "WarOfDimension.vcxproj", "Shader.cpp", "Shader.h", "DDSTextureLoader12.cpp", "ShadowMap.h", "Scene.cpp")]
    source_paths += ["scripts/Convert-UiTextures.py", "scripts/Capture-UiTextures.ps1", "scripts/Build-UiTextureEvidence.py"]
    report = dict(schemaVersion=1, implemented=True, units="bytes; MiB = bytes / 1048576",
                  scope="CTextureShader INGAME UI: 20 unique DDS per role, 21 files across both roles",
                  comparison="Same Release executable and UI shader; original assets then BC7 assets, 3 sequential processes each; fixed offline loadout and synthetic particle replay, not network combat",
                  conversion=conversion, native=native, memory=memory, textureQuality=texture_quality, screenshots=screenshots,
                  sourceSha256={p:sha(ROOT/p) for p in source_paths})
    (out/"measurements.json").write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    with (out/"memory.csv").open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["phase", "run", *fields]); writer.writeheader(); writer.writerows(rows)
    options = "\n".join(f'<option value="{i}">{s["role"]} · {s["file"]}</option>' for i,s in enumerate(screenshots))
    gallery = '''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>UI BC7 전후 GPU 화면</title><style>body{margin:0;padding:24px;background:#161a20;color:#edf1f7;font:16px system-ui}h1{font-size:24px}select{padding:10px;max-width:100%;font-size:16px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:20px}img{width:100%;height:auto;border:1px solid #526074;box-sizing:border-box}a{color:#89c6ff}figure{margin:0}figcaption{padding:8px 0}@media(max-width:800px){.grid{grid-template-columns:1fr}}</style>
<h1>UI BC7 전후 GPU 화면</h1><p>실제 CTextureShader·UI 객체의 고정 시간 렌더링. 맵·네트워크 전투를 제외한 UI 비교입니다. 원본 PNG는 1920×1080이며 링크에서 확대할 수 있습니다.</p>
<label>비교 화면 <select id="view">OPTIONS</select></label><div class="grid"><figure><figcaption>변경 전 · RGBA32</figcaption><a id="beforeLink"><img id="before" alt="변경 전 UI"></a></figure><figure><figcaption>변경 후 · BC7</figcaption><a id="afterLink"><img id="after" alt="변경 후 UI"></a></figure></div><p id="detail"></p>
<script>const views=DATA;const select=document.querySelector('#view');function update(){const v=views[Number(select.value)];for(const phase of ['before','after']){document.getElementById(phase).src=v.images[phase].path;document.getElementById(phase+'Link').href=v.images[phase].path;}document.querySelector('#detail').textContent='대상 '+v.role+' / '+v.file+' · RGB foreground PSNR(dB): '+v.foreground.psnrDb.map(x=>x===null?'동일':x.toFixed(2)).join(' / ');}select.addEventListener('change',update);update();</script></html>'''
    gallery = gallery.replace("OPTIONS", options).replace("DATA", json.dumps(screenshots, ensure_ascii=False))
    (out/"gallery.html").write_text(gallery, encoding="utf-8")
    print("PASS: 21 DDS 원본/압축 해시·내용 픽셀 보존, 32 전후 GPU 화면 쌍, 6 메모리 실행, 역할별 수명/실패 검사")


if __name__ == "__main__":
    main()
