param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$report = Join-Path $script:RepoRoot "artifacts/logs/test-object-constants-$Configuration.json"
$process = Start-Process -FilePath (Get-WodExecutable Client $Configuration) -ArgumentList @('--test-object-constants', ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(60000)) { throw '상수 arena GPU 검사 시간 초과' }
    if ($process.ExitCode -ne 0) {
        if (Test-Path "$report.error.txt") { Get-Content "$report.error.txt" -Encoding utf8 | Write-Host }
        throw "상수 arena GPU 검사 실패: $report"
    }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $data.ok -or $data.checks -ne 10 -or $data.liveAfterExit -ne 0 -or $data.gpuErrors -ne 0) { throw 'arena 검사 결과 미완료' }
    Write-Host "상수 arena PASS ($Configuration): draw 독립성·2-frame fence·확장·GPU readback·종료 해제, warnings=$($data.gpuWarnings)"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
