param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [string] $Python = '',
    [switch] $Build
)
. "$PSScriptRoot/Development.ps1"
$pythonCommand = Get-WodPython $Python
& $pythonCommand -m unittest discover -s "$script:RepoRoot/tools/server_lab/tests" -v
if ($LASTEXITCODE -ne 0) { throw 'Server Lab 단위 검증 실패' }
& "$PSScriptRoot/Start-ServerLab.ps1" -Configuration $Configuration -Python $pythonCommand -Build:$Build
try {
    $output = Join-Path $script:RepoRoot "artifacts/logs/server-lab/integration-$Configuration.json"
    & $pythonCommand "$script:RepoRoot/tools/server_lab/tests/integration.py" --output $output
    if ($LASTEXITCODE -ne 0) { throw "Server Lab 통합 검증 실패: $output" }
} finally {
    & "$PSScriptRoot/Stop-ServerLab.ps1"
}
foreach ($role in @('LobbyServer', 'GameServer')) {
    $log = Join-Path $script:RepoRoot "artifacts/logs/run-$role.err.log"
    if ((Get-Content -LiteralPath $log -Raw) -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "$role 종료 자원 검증 실패" }
    $sample = Get-Content -LiteralPath "$script:RepoRoot/artifacts/logs/server-lab/metrics/$role.json" -Raw | ConvertFrom-Json
    if (-not $sample.stopped) { throw "$role 최종 계측 검증 실패" }
}
# 내보내기 경로를 디렉터리 대신 파일로 만들어 계측 I/O 실패를 주입한다.
$blockedPath = Join-Path $script:RepoRoot ('.runtime/server-lab-export-' + [Guid]::NewGuid().ToString('N'))
$previousDirectory = $env:WOD_METRICS_DIRECTORY
$serversStarted = $false
try {
    [IO.File]::WriteAllText($blockedPath, 'export failure fixture')
    $env:WOD_METRICS_DIRECTORY = $blockedPath
    & "$PSScriptRoot/Start-Local.ps1" -Configuration $Configuration -ServersOnly
    $serversStarted = $true
    & $pythonCommand "$script:RepoRoot/tools/server_lab/dummy_client.py" --scenario "$script:RepoRoot/tools/server_lab/scenarios/lobby-cycle.json"
    if ($LASTEXITCODE -ne 0) { throw '계측 I/O 실패 중 로그인·재접속 검증 실패' }
} finally {
    try {
        if ($serversStarted) { & "$PSScriptRoot/Stop-Local.ps1" }
    } finally {
        $env:WOD_METRICS_DIRECTORY = $previousDirectory
        Remove-Item -LiteralPath $blockedPath -ErrorAction SilentlyContinue
    }
}
foreach ($role in @('LobbyServer', 'GameServer')) {
    $log = Join-Path $script:RepoRoot "artifacts/logs/run-$role.err.log"
    $content = Get-Content -LiteralPath $log -Raw
    if ([regex]::Matches($content, 'Telemetry export failed:').Count -ne 1 -or $content -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "$role 계측 실패 격리/정상 종료 검증 실패" }
    Copy-Item -LiteralPath $log -Destination "$script:RepoRoot/artifacts/logs/server-lab/export-failure-$Configuration-$role.log"
}
Write-Host "Server Lab 단위·실제 서버/API·정상 종료 검증: PASS ($Configuration)"
