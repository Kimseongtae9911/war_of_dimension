param(
    [ValidateRange(1,3600)][int] $TimeoutSeconds = 15,
    [switch] $Force
)
. "$PSScriptRoot/Development.ps1"
$statePath = Join-Path $script:RepoRoot '.runtime/processes.json'
if (-not (Test-Path -LiteralPath $statePath)) { Write-Host '이 스크립트로 시작한 실행 기록이 없습니다.'; return }
$records = @(Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json)
[array]::Reverse($records)
$failures = @()
$remaining = [Collections.Generic.List[object]]::new()
foreach ($record in $records) {
    $process = Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue
    if (-not $process) {
        if ($record.Module -in @('LobbyServer','GameServer')) {
            if ($record.PSObject.Properties.Name -contains 'StopRequested' -and $record.StopRequested) {
                Write-Warning "$($record.Module): 이전 종료 요청 후 프로세스 종료 확인, 종료 코드는 로그에서 확인하세요."
            } else {
                $failures += "$($record.Module): 정상 종료 요청 전에 프로세스가 종료됨"
            }
        }
        continue
    }
    $null = $process.Handle
    $expectedStart = if ($record.StartTimeUtc -is [DateTime]) { $record.StartTimeUtc.ToUniversalTime() } else { [DateTime]::Parse($record.StartTimeUtc, [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime() }
    if ($process.Path -ne $record.Executable -or $process.StartTime.ToUniversalTime().Ticks -ne $expectedStart.Ticks) { throw "PID $($record.ProcessId)의 실행 파일 또는 시작 시간이 변경되어 중지하지 않습니다." }
    if ($record.Module -in @('LobbyServer','GameServer')) {
        try {
            $signal = [Threading.EventWaitHandle]::OpenExisting("Local\Wod.Server.Stop.$($record.ProcessId)")
            try { $null = $signal.Set() } finally { $signal.Dispose() }
            $record | Add-Member -NotePropertyName StopRequested -NotePropertyValue $true -Force
            if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { throw '정상 종료 대기 시간 초과; 처리 중인 작업을 보존합니다.' }
            $process.Refresh()
            if ($process.ExitCode -ne 0) { $failures += "$($record.Module) 종료 코드: $($process.ExitCode)" }
        } catch {
            $failures += "$($record.Module): $($_.Exception.Message)"
            if (-not $process.HasExited) {
                if ($Force) {
                    Stop-Process -Id $record.ProcessId
                    Write-Warning "$($record.Module): -Force로 강제 종료했습니다. 미완료 요청은 복구 기록을 확인하세요."
                } else {
                    $remaining.Add($record)
                    Write-Warning "$($record.Module): 프로세스와 실행 기록을 유지합니다. 기다린 뒤 Stop-Local.ps1을 다시 실행하세요."
                }
            }
        }
    } else {
        Stop-Process -Id $record.ProcessId
    }
    if (-not ($remaining | Where-Object { $_.ProcessId -eq $record.ProcessId })) { Write-Host "중지 완료: $($record.Module)" }
}
if ($remaining.Count) {
    $remaining.ToArray() | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $statePath -Encoding utf8
} else {
    Remove-Item -LiteralPath $statePath
}
if ($failures.Count) { throw ('정상 종료 실패: ' + ($failures -join '; ')) }
