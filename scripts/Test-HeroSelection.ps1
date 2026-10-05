param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$executable = Get-WodExecutable Client $Configuration
$report = Join-Path $script:RepoRoot "artifacts/logs/test-hero-selection-$Configuration.json"
$process = Start-Process -FilePath $executable -ArgumentList @('--test-hero-selection', ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(180000)) { throw '영웅 파츠 검증 시간 초과' }
    if ($process.ExitCode -ne 0) { throw "영웅 파츠 검증 실패: exit $($process.ExitCode), $report" }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $data.ok -or $data.comparedMatrixRows -lt 1) { throw '영웅 파츠 검증 결과 미완료' }
    Write-Host "영웅 선택 파츠 검증: PASS ($Configuration), $($data.comparedMatrixRows)개 실제 행렬 비교"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
