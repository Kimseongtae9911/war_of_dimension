param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [switch] $AuditAssets
)
. "$PSScriptRoot/Development.ps1"
$executable = Get-WodExecutable Client $Configuration
if (-not (Test-Path -LiteralPath $executable)) { throw '먼저 scripts/Build.ps1을 실행하세요.' }
$logRoot = Join-Path $script:RepoRoot 'artifacts/logs'
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
$modes = @('test-mesh-sharing')
if ($AuditAssets) { $modes += 'audit-mesh-assets' }
foreach ($mode in $modes) {
    $report = Join-Path $logRoot "$mode-$Configuration.json"
    $process = Start-Process -FilePath $executable -ArgumentList @("--$mode", ('"' + $report + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(120000)) { $process.Kill(); throw "메시 점검 시간 초과: $mode" }
    if ($process.ExitCode -ne 0) {
        if (Test-Path -LiteralPath $report) { Get-Content -LiteralPath $report }
        throw "메시 점검 실패: $mode (exit $($process.ExitCode))"
    }
    $result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (-not $result.ok) { throw "메시 점검 결과 실패: $report" }
    Write-Host "$mode : PASS ($Configuration). $report"
}
