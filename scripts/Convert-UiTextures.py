"""원본 UI 픽셀·wrap 경계를 보존해 BC7 DDS로 변환한다. 적용은 --apply로 명시한다."""
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
TOOL_SHA = "dcfdec10244e02cf5037fba089c55fb7e1326b1c8181742d77d15fa5cb5eef06"
MAGIC = int.from_bytes(b"UIB7", "little")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "artifacts/logs/ui-bc7-before/Client/WarOfDimension")
    parser.add_argument("--output", type=Path, default=ROOT / "artifacts/logs/ui-bc7-converted")
    parser.add_argument("--texconv", type=Path, default=ROOT / "artifacts/tools/directxtex-may2026/texconv.exe")
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    if sha(args.texconv) != TOOL_SHA:
        raise ValueError("고정 Microsoft DirectXTex may2026 도구 해시 불일치")
    estimate = json.loads((ROOT / "docs/portfolio/evidence/ui-bc7-estimate-20261005.json").read_text(encoding="utf-8"))
    results = []
    for item in estimate["textures"]:
        source = args.source / item["path"]
        if sha(source) != item["sha256"]:
            raise ValueError(f"원본 DDS 해시 불일치: {source}")
        raw = source.read_bytes()
        width, height = item["width"], item["height"]
        if len(raw) != 128 + width * height * 4 or raw[84:88] != bytes(4):
            raise ValueError(f"예상한 단일 mip RGBA32 DDS가 아님: {source}")
        # 원본 texel의 bit 배치를 그대로 유지한다. 1px wrap gutter는 bilinear
        # sampling의 양쪽 경계도 기존 wrap sampler와 같게 만든다.
        offset = 1 if width % 4 or height % 4 else 0
        padded_w = (width + 2 * offset + 3) // 4 * 4
        padded_h = (height + 2 * offset + 3) // 4 * 4
        pixels = np.frombuffer(raw, "<u4", offset=128).reshape(height, width)
        padded = np.pad(pixels, ((offset, padded_h - height - offset),
                                (offset, padded_w - width - offset)), mode="wrap")
        header = bytearray(raw[:128])
        struct.pack_into("<III", header, 12, padded_h, padded_w, padded_w * 4)
        temporary = args.output / "padded" / item["path"]
        converted = args.output / "converted" / item["path"]
        temporary.parent.mkdir(parents=True, exist_ok=True)
        converted.parent.mkdir(parents=True, exist_ok=True)
        temporary.write_bytes(header + padded.astype("<u4", copy=False).tobytes())
        command = [str(args.texconv.resolve()), "-nologo", "-y", "-m", "1", "-f", "BC7_UNORM",
                   "-bc", "x", "-gpu", "0", "-o", str(converted.parent.resolve()), str(temporary.resolve())]
        run = subprocess.run(command, capture_output=True, text=True, errors="replace")
        converted.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
        if run.returncode:
            raise RuntimeError(f"BC7 변환 실패: {source}\n{run.stdout}\n{run.stderr}")
        data = bytearray(converted.read_bytes())
        if (data[84:88] != b"DX10" or struct.unpack_from("<I", data, 128)[0] != 98
                or struct.unpack_from("<II", data, 12) != (padded_h, padded_w)
                or max(1, struct.unpack_from("<I", data, 28)[0]) != 1
                or len(data) != 148 + padded_w * padded_h):
            raise ValueError(f"BC7 DDS 헤더/payload 검증 실패: {converted}")
        # DDS reserved1: 프로젝트 UI 내용 영역 계약. DirectX loader는 이 필드를
        # 무시하며 공통 helper가 같은 파일 bytes에서 UV scale/offset만 읽는다.
        struct.pack_into("<6I", data, 32, MAGIC, 1, width, height, offset, offset)
        converted.write_bytes(data)
        results.append(dict(path=item["path"], beforeSha256=item["sha256"], afterSha256=sha(converted),
                            width=width, height=height, paddedWidth=padded_w, paddedHeight=padded_h,
                            offsetX=offset, offsetY=offset, mipLevels=1, format="BC7_UNORM",
                            beforePayloadBytes=width * height * 4, afterPayloadBytes=padded_w * padded_h,
                            beforeFileBytes=len(raw), afterFileBytes=len(data)))
        print(f"BC7 검증 PASS: {item['path']} {width}x{height} -> {padded_w}x{padded_h}", flush=True)
    # 전 파일 변환·검증 성공 뒤에만 적용한다. 원본은 별도 보존되어 있다.
    if args.apply:
        for item in results:
            shutil.copy2(args.output / "converted" / item["path"], ROOT / "Client/WarOfDimension" / item["path"])
        subprocess.run(["git", "lfs", "track", *["Client/WarOfDimension/" + t["path"] for t in results]], cwd=ROOT, check=True)
    report = dict(schemaVersion=1, applied=args.apply, toolRelease="Microsoft DirectXTex may2026",
                  toolSha256=TOOL_SHA, officialSource="https://github.com/microsoft/DirectXTex/releases/tag/may2026",
                  arguments=["-m", "1", "-f", "BC7_UNORM", "-bc", "x", "-gpu", "0"],
                  layoutVersion=1, padding="1px wrap gutter only when either original dimension is not 4-aligned",
                  textures=results)
    (args.output / "conversion.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
