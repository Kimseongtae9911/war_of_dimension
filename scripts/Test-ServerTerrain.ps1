param([ValidateSet('Debug','Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
& "$PSScriptRoot/Test-ServerCore.ps1" -Configuration $Configuration
$logs = Join-Path $script:RepoRoot "artifacts/logs/test-server-terrain-$Configuration"
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$exe = Join-Path $script:RepoRoot "artifacts/bin/$Configuration/ServerCore.Tests/ServerCore.Tests.exe"
$process = Start-Process -FilePath $exe -ArgumentList '--terrain-assets' -WorkingDirectory $script:RepoRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $logs 'assets.json') -RedirectStandardError (Join-Path $logs 'assets.stderr.log')
if (-not $process.WaitForExit(300000)) { Stop-Process -Id $process.Id; throw '지형 등가 검사 timeout' }
$process.Refresh()
if ($process.ExitCode -ne 0) { throw "지형 등가 검사 실패: $logs" }
$result = Get-Content (Join-Path $logs 'assets.json') -Raw | ConvertFrom-Json
if (-not $result.ok -or $result.meshes.Count -ne 2 -or $result.checks -le 0) { throw '지형 검사 결과 누락' }
Write-Host "실제 NavMesh·HeightMesh first-hit/높이/node 등가 검사: PASS ($Configuration, $($result.checks)비교)"
