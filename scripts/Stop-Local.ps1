param()
. "$PSScriptRoot/Development.ps1"
$statePath = Join-Path $script:RepoRoot '.runtime/processes.json'
if (-not (Test-Path -LiteralPath $statePath)) { Write-Host '이 스크립트로 시작한 실행 기록이 없습니다.'; return }
$records = @(Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json)
[array]::Reverse($records)
$failures = @()
foreach ($record in $records) {
    $process = Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue
    if (-not $process) {
        if ($record.Module -in @('LobbyServer','GameServer')) { $failures += "$($record.Module): 정상 종료 요청 전에 프로세스가 종료됨" }
        continue
    }
    # Get-Process가 exit 후 핸들을 처음 열면 ExitCode를 얻지 못한다. 먼저 핸들을 확보한다.
    $null = $process.Handle
    # PowerShell 7.5 이후 ConvertFrom-Json은 ISO 날짜를 DateTime으로 해석할 수 있다.
    $expectedStart = if ($record.StartTimeUtc -is [DateTime]) { $record.StartTimeUtc.ToUniversalTime() } else { [DateTime]::Parse($record.StartTimeUtc, [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime() }
    if ($process.Path -ne $record.Executable -or $process.StartTime.ToUniversalTime().Ticks -ne $expectedStart.Ticks) { throw "PID $($record.ProcessId)의 실행 파일 또는 시작 시간이 변경되어 중지하지 않습니다." }
    if ($record.Module -in @('LobbyServer','GameServer')) {
        try {
            $signal = [Threading.EventWaitHandle]::OpenExisting("Local\Wod.Server.Stop.$($record.ProcessId)")
            try { $null = $signal.Set() } finally { $signal.Dispose() }
            if (-not $process.WaitForExit(15000)) { throw '정상 종료 대기 시간 초과' }
            $process.Refresh()
            if ($process.ExitCode -ne 0) { $failures += "$($record.Module) 종료 코드: $($process.ExitCode)" }
        } catch {
            $failures += "$($record.Module): $($_.Exception.Message)"
            if (-not $process.HasExited) { Stop-Process -Id $record.ProcessId }
        }
    } else {
        Stop-Process -Id $record.ProcessId
    }
    Write-Host "중지 완료: $($record.Module)"
}
Remove-Item -LiteralPath $statePath
if ($failures.Count) { throw ('정상 종료 실패: ' + ($failures -join '; ')) }
