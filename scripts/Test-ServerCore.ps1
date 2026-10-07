param([ValidateSet('Debug','Release')][string] $Configuration = 'Debug')
. "$PSScriptRoot/Development.ps1"
$project = Join-Path $script:RepoRoot 'Server/ServerCore/tests/ServerCore.Tests.vcxproj'
$log = Join-Path $script:RepoRoot "artifacts/logs/test-server-core-$Configuration"
New-Item -ItemType Directory -Force -Path $log | Out-Null
& (Get-WodMSBuild) $project "/p:Configuration=$Configuration" '/p:Platform=x64' '/nologo' '/verbosity:minimal'
if ($LASTEXITCODE -ne 0) { throw 'ServerCore 테스트 빌드 실패' }
$exe = Join-Path $script:RepoRoot "artifacts/bin/$Configuration/ServerCore.Tests/ServerCore.Tests.exe"
foreach ($mode in @('checks','abi')) {
    $parameters = @{ FilePath=$exe; WorkingDirectory=$script:RepoRoot; WindowStyle='Hidden'; PassThru=$true; RedirectStandardOutput=(Join-Path $log "$mode.json"); RedirectStandardError=(Join-Path $log "$mode.stderr.log") }
    if ($mode -eq 'abi') { $parameters.ArgumentList = @('--abi') }
    $process = Start-Process @parameters
    if (-not $process.WaitForExit(30000)) { Stop-Process -Id $process.Id; throw "ServerCore $mode timeout" }
    $process.Refresh()
    if ($process.ExitCode -ne 0) { throw "ServerCore $mode 실패: $log" }
}
$actual = Get-Content (Join-Path $log 'abi.json') -Raw | ConvertFrom-Json
$expected = Get-Content (Join-Path $script:RepoRoot 'Shared/Protocol/tests/abi-x64-msvc.json') -Raw | ConvertFrom-Json
if (($actual | ConvertTo-Json -Depth 15 -Compress) -ne ($expected | ConvertTo-Json -Depth 15 -Compress)) { throw '기존 protocol ABI/상수 변경' }
Write-Host "ServerCore 테스트·Protocol 110종/106상수: PASS ($Configuration)"
