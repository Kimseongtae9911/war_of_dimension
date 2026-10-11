param()
. "$PSScriptRoot/Development.ps1"
$labState = Join-Path $script:RepoRoot '.runtime/server-lab.json'
if (-not (Test-Path -LiteralPath $labState)) { Write-Host 'Server Lab 실행 기록이 없습니다.'; return }
$record = Get-Content -LiteralPath $labState -Raw | ConvertFrom-Json
$monitorProcess = Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue
if ($monitorProcess) {
    $null = $monitorProcess.Handle
    $expectedStart = if ($record.StartTimeUtc -is [DateTime]) { $record.StartTimeUtc.ToUniversalTime() } else { [DateTime]::Parse($record.StartTimeUtc).ToUniversalTime() }
    if ($monitorProcess.Path -ne $record.Executable -or $monitorProcess.StartTime.ToUniversalTime().Ticks -ne $expectedStart.Ticks) { throw 'Server Lab PID 소유권이 달라 종료하지 않습니다.' }
    Invoke-RestMethod -Uri "http://127.0.0.1:$($record.Port)/api/shutdown" -Method Post -ContentType 'application/json' -Body '{}' -TimeoutSec 5 | Out-Null
    if (-not $monitorProcess.WaitForExit(15000)) { throw 'Server Lab 정상 종료 시간 초과' }
    $monitorProcess.Refresh()
    if ($monitorProcess.ExitCode -ne 0) { throw "Server Lab 종료 코드: $($monitorProcess.ExitCode)" }
}
try {
    if ($record.StartedServers) { & "$PSScriptRoot/Stop-Local.ps1" }
} finally {
    # 관측 프로세스는 이미 종료했다. 서버 종료 실패도 전달하되 실행 기록은 남기지 않는다.
    Remove-Item -LiteralPath $labState -ErrorAction SilentlyContinue
}
Write-Host 'Server Lab 중지 완료'
