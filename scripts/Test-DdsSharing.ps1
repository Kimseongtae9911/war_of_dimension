param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$report = Join-Path $script:RepoRoot "artifacts/logs/test-dds-sharing-$Configuration.json"
$process = Start-Process -FilePath (Get-WodExecutable Client $Configuration) -ArgumentList @('--test-dds-sharing', ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(60000)) { throw 'DDS 공유 GPU 검사 시간 초과' }
    if ($process.ExitCode -ne 0) {
        if (Test-Path "$report.error.txt") { Get-Content "$report.error.txt" -Encoding utf8 | Write-Host }
        throw "DDS 공유 GPU 검사 실패: $report"
    }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $data.ok -or $data.checks -ne 12 -or $data.uniqueResources -ne 2 -or $data.liveAfterExit -ne 0 -or $data.gpuErrors -ne 0) { throw 'DDS 공유/수명 검사 결과 미완료' }
    Write-Host "DDS 공유 검사 PASS ($Configuration): 실제 모델 texture 4→2, DEFAULT/UPLOAD 각 $($data.savedDefaultBytes / 1MB)MiB 절감, GPU warnings=$($data.gpuWarnings)"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
