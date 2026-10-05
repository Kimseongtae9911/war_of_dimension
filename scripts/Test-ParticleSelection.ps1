param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$report = Join-Path $script:RepoRoot "artifacts/logs/test-particle-selection-$Configuration.json"
$process = Start-Process -FilePath (Get-WodExecutable Client $Configuration) -ArgumentList @('--test-particle-selection', ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(30000)) { throw '선택 스킬 파티클 검사 시간 초과' }
    if ($process.ExitCode -ne 0) { throw "선택 스킬 파티클 검사 실패: $report" }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $data.ok -or $data.incompleteSlotCases -ne 16) { throw '검사 결과 미완료' }
    Write-Host "선택 스킬 파티클 검사 PASS ($Configuration): 전체 풀 $($data.fullPoolCount) → 선택 풀 $($data.selectedPoolCount)"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
