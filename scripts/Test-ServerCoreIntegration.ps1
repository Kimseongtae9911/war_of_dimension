param(
    [ValidateSet('Debug','Release')][string] $Configuration = 'Debug',
    [string] $Python = 'python'
)
. "$PSScriptRoot/Development.ps1"
$logs = Join-Path $script:RepoRoot "artifacts/logs/server-core-integration-$Configuration"
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$env:PYTHONIOENCODING = 'utf-8'
$results = [Collections.Generic.List[object]]::new()
foreach ($case in @('network','boss4','boss5')) {
    & "$PSScriptRoot/Start-Local.ps1" -Configuration $Configuration -ServersOnly
    try {
        $output = Join-Path $logs "$case.json"
        if ($case -eq 'network') {
            & $Python "$PSScriptRoot/Test-ServerCoreNetwork.py" --output $output
        } else {
            $boss = [int]$case.Substring(4)
            & $Python "$PSScriptRoot/Test-ParticleSelectionNetwork.py" --boss-job $boss --load-complete --output $output
        }
        if ($LASTEXITCODE -ne 0) { throw "ServerCore 통합 회귀 실패: $case" }
        # 첫 NPC 생성 예약 이후에도 서버가 살아 있는지 확인한다.
        Start-Sleep -Seconds 3
    } finally {
        & "$PSScriptRoot/Stop-Local.ps1"
        foreach ($module in @('LobbyServer','GameServer')) {
            $source = Join-Path $script:RepoRoot "artifacts/logs/run-$module.err.log"
            Copy-Item -LiteralPath $source -Destination (Join-Path $logs "$case-$module.err.log")
            if ((Get-Content -LiteralPath $source -Raw) -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "$module 종료 자원 검증 실패: $case" }
        }
    }
    $results.Add(@{Case=$case; Passed=$true; NormalStop=$true; Pending=0; Sockets=0; Leased=0})
}
# 네트워크를 시작하기 전·도중 실패해도 Core 소유 자원은 모두 회수해야 한다.
$empty = Join-Path $script:RepoRoot 'artifacts/server-core-empty-workdir'
New-Item -ItemType Directory -Force -Path $empty | Out-Null
foreach ($case in @('LobbyServer-missing-resource','GameServer-missing-resource','GameServer-lobby-offline','LobbyServer-bind-conflict')) {
    $module = $case.Split('-')[0]
    $working = if ($case.EndsWith('missing-resource')) { $empty } else { Join-Path $script:RepoRoot ($(if ($module -eq 'GameServer') {'Server/Game_Server'} else {'Server/Lobby_Server'})) }
    $listener = $null
    if ($case.EndsWith('bind-conflict')) {
        $listener=[Net.Sockets.TcpListener]::new([Net.IPAddress]::Any,8910)
        $listener.Server.ExclusiveAddressUse = $true
        $listener.Start()
    }
    try {
        $stderr = Join-Path $logs "$case.err.log"
        $process = Start-Process -FilePath (Get-WodExecutable $module $Configuration) -WorkingDirectory $working -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $logs "$case.out.log") -RedirectStandardError $stderr
        $null = $process.Handle
        if (-not $process.WaitForExit(30000)) { Stop-Process -Id $process.Id; throw "초기화 실패 회수 timeout: $case" }
        $process.Refresh()
        if ($process.ExitCode -ne 1) { throw "초기화 실패 종료 코드: $case / $($process.ExitCode)" }
        if ((Get-Content -LiteralPath $stderr -Raw) -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "초기화 실패 자원 회수 실패: $case" }
        $results.Add(@{Case=$case; Passed=$true; ExpectedExitCode=1; Pending=0; Sockets=0; Leased=0})
    } finally { if ($listener) { $listener.Stop() } }
}
@{Ok=$true;Configuration=$Configuration;LocalTest=$true;Database=$false;Cases=$results.ToArray()} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $logs 'summary.json') -Encoding utf8
Write-Host "ServerCore 통합·초기화 실패 회귀: PASS ($Configuration)"
