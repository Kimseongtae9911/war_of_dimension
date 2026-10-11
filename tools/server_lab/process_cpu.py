"""실행 기록의 PID/생성 시각이 일치하는 Windows 서버의 CPU 누적 시간. 계측 끔 대조에도 사용한다."""
import ctypes
import datetime
import json
from ctypes import wintypes
from pathlib import Path


def sample(directory):
    path = Path(directory) / '.runtime/processes.json'
    if not path.exists():
        return {}
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE, *([ctypes.POINTER(wintypes.FILETIME)] * 4)]
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    output = {}
    for record in json.loads(path.read_text(encoding='utf-8-sig')):
        handle = kernel.OpenProcess(0x1000, False, record['ProcessId'])
        if not handle:
            continue
        try:
            created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
            if not kernel.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited), ctypes.byref(system), ctypes.byref(user)):
                continue
            ticks = lambda value: (value.dwHighDateTime << 32) | value.dwLowDateTime
            expected = datetime.datetime.fromisoformat(record['StartTimeUtc'].replace('Z', '+00:00')).timestamp()
            if abs(ticks(created) - (int(expected * 10_000_000) + 116444736000000000)) > 20:
                continue
            output[record['Module']] = {'pid': record['ProcessId'], 'creation_ticks': ticks(created), 'cpu_ticks': ticks(system) + ticks(user)}
        finally:
            kernel.CloseHandle(handle)
    return output
