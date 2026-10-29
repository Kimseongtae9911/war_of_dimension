param([switch] $Build)
. "$PSScriptRoot/Development.ps1"
if ($Build) { & "$PSScriptRoot/Build.ps1" -Module Servers -Configuration Release }
$logRoot = Join-Path $script:RepoRoot 'artifacts/logs/database'
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
$process = Start-Process -FilePath (Join-Path $script:RepoRoot 'artifacts/bin/Release/ServerCore.Tests/ServerCore.Tests.exe') -ArgumentList '--db-benchmark' -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $logRoot 'benchmark-Release.json') -RedirectStandardError (Join-Path $logRoot 'benchmark-Release.err.log')
$null = $process.Handle
if (-not $process.WaitForExit(15000)) { Stop-Process -Id $process.Id; throw 'DB 경계 측정 timeout' }
$process.Refresh()
if ($process.ExitCode -ne 0) { throw 'DB 경계 측정 실패' }
Write-Host 'DB 경계 전후 교대 5회 + 워밍업 1회: 완료'
