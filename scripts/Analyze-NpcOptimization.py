"""BC7 전후 DDS 채널·실제 GPU 캡처의 픽셀 차이를 수치로 검증한다."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

import numpy as np
from PIL import Image


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decoded_dds(path):
    data = path.read_bytes()
    height, width = struct.unpack_from("<II", data, 12)
    offset = 148 if data[84:88] == b"DX10" else 128
    if offset == 148:
        assert struct.unpack_from("<I", data, 128)[0] == 28
    else:
        assert struct.unpack_from("<IIII", data, 92) == (255, 65280, 16711680, 4278190080)
    return np.frombuffer(data, np.uint8, offset=offset).reshape(height, width, 4)


def error_metrics(before, after, mask=None):
    assert before.shape == after.shape
    squared = np.zeros(before.shape[-1], np.float64)
    maxima = np.zeros(before.shape[-1], np.int32)
    count = changed = 0
    for row in range(0, before.shape[0], 256):
        a, b = before[row:row+256], after[row:row+256]
        if mask is not None:
            a, b = a[mask[row:row+256]], b[mask[row:row+256]]
        difference = a.astype(np.float32) - b.astype(np.float32)
        difference = difference.reshape(-1, before.shape[-1])
        squared += np.sum(difference * difference, axis=0, dtype=np.float64)
        maxima = np.maximum(maxima, np.max(np.abs(difference), axis=0, initial=0).astype(np.int32))
        count += difference.shape[0]
        changed += int(np.count_nonzero(np.any(difference != 0, axis=-1)))
    mse = squared / count
    return dict(pixels=count, changedPixels=changed, equal=changed == 0,
                mse=mse.tolist(), maxAbsoluteError=maxima.tolist(),
                psnrDb=[None if value == 0 else 10 * math.log10(255**2 / value) for value in mse])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--before", default="artifacts/logs/npc-bc7-before")
    parser.add_argument("--after", default="artifacts/logs/npc-bc7-after")
    parser.add_argument("--decoded", default="artifacts/logs/npc-texture-decoded")
    parser.add_argument("--output", default="artifacts/logs/npc-quality.json")
    args = parser.parse_args()
    before, after, decoded = map(Path, (args.before, args.after, args.decoded))
    a, b = read_json(before / "capture.json"), read_json(after / "capture.json")
    assert a["views"] == b["views"], "전후 포즈/카메라가 다름"
    review = read_json(Path("docs/portfolio/evidence/npc-sharing-review-20261005.json"))
    textures = []
    for metadata in review["textures"].values():
        name = Path(metadata["path"]).name
        item = dict(path=metadata["path"], **error_metrics(decoded_dds(decoded / "before" / name), decoded_dds(decoded / "after" / name)))
        textures.append(item)
    screenshots = []
    for view in a["views"]:
        name = view["file"]
        original = np.array(Image.open(before / name).convert("RGB"))
        optimized = np.array(Image.open(after / name).convert("RGB"))
        mask = np.any(original != 0, axis=-1) | np.any(optimized != 0, axis=-1)
        assert np.count_nonzero(mask) > 10000, "빈 모델 캡처"
        screenshots.append(dict(file=name, beforeSha256=sha(before/name), afterSha256=sha(after/name),
                                foreground=error_metrics(original, optimized, mask)))
    result = dict(schemaVersion=1, samePoseCamera=True, textureChannels="RGBA", screenshotChannels="RGB",
                  psnrIdenticalChannel=None, screenshots=screenshots, textures=textures)
    Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("DDS 25개 RGBA·실제 GPU 화면 18장 전후 분석 완료")
    print("최저 DDS 채널 PSNR:", min(value for item in textures for value in item["psnrDb"] if value is not None))
    print("최저 화면 foreground 채널 PSNR:", min(value for item in screenshots for value in item["foreground"]["psnrDb"] if value is not None))


if __name__ == "__main__":
    main()
