param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$report = Join-Path $script:RepoRoot "artifacts/logs/test-particle-buffer-pool-$Configuration.json"
$process = Start-Process -FilePath (Get-WodExecutable Client $Configuration) -ArgumentList @('--test-particle-buffer-pool', ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(45000)) { throw '파티클 GPU 풀 검사 시간 초과' }
    if ($process.ExitCode -ne 0) {
        if (Test-Path "$report.error.txt") { Get-Content "$report.error.txt" -Encoding utf8 | Write-Host }
        throw "파티클 GPU 풀 검사 실패: $report"
    }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $data.ok -or $data.cycles -ne 2 -or $data.livePairsAfterExit -ne 0 -or $data.gpuErrors -ne 0) { throw '풀 수명/확장 검사 결과 미완료' }
    Write-Host "파티클 GPU 풀 검사 PASS ($Configuration): 확장·종류 간 재사용·상태 보존·fence·두 번 종료, GPU warnings=$($data.gpuWarnings)"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
