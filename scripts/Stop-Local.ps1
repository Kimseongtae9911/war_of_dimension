param()
. "$PSScriptRoot/Development.ps1"
$statePath = Join-Path $script:RepoRoot '.runtime/processes.json'
if (-not (Test-Path -LiteralPath $statePath)) { Write-Host '이 스크립트로 시작한 실행 기록이 없습니다.'; return }
$records = @(Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json)
[array]::Reverse($records)
foreach ($record in $records) {
    $process = Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue
    if (-not $process) { continue }
    # PowerShell 7.5 이후 ConvertFrom-Json은 ISO 날짜를 DateTime으로 해석할 수 있다.
    $expectedStart = if ($record.StartTimeUtc -is [DateTime]) { $record.StartTimeUtc.ToUniversalTime() } else { [DateTime]::Parse($record.StartTimeUtc, [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime() }
    if ($process.Path -ne $record.Executable -or $process.StartTime.ToUniversalTime().Ticks -ne $expectedStart.Ticks) { throw "PID $($record.ProcessId)의 실행 파일 또는 시작 시간이 변경되어 중지하지 않습니다." }
    Stop-Process -Id $record.ProcessId
    Write-Host "중지 완료: $($record.Module)"
}
Remove-Item -LiteralPath $statePath
