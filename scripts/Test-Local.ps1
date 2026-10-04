param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [switch] $ServersOnly
)
. "$PSScriptRoot/Development.ps1"
& "$PSScriptRoot/Start-Local.ps1" -Configuration $Configuration -ServersOnly:$ServersOnly
try {
    $records = @(Get-Content -LiteralPath (Join-Path $script:RepoRoot '.runtime/processes.json') -Raw | ConvertFrom-Json)
    $game = @($records | Where-Object { $_.Module -eq 'GameServer' })[0]
    $connection = Get-NetTCPConnection -State Established -RemotePort 8910 -ErrorAction SilentlyContinue | Where-Object { $_.OwningProcess -eq $game.ProcessId }
    if (-not $connection) { throw '로비·게임 서버의 TCP 연결을 확인하지 못했습니다.' }
    if (-not $ServersOnly) {
        $clientRecord = @($records | Where-Object { $_.Module -eq 'Client' })[0]
        $client = Get-Process -Id $clientRecord.ProcessId
        $deadline = [DateTime]::UtcNow.AddSeconds(60)
        while ([DateTime]::UtcNow -lt $deadline) {
            $client.Refresh()
            if ($client.HasExited) { throw '클라이언트가 초기화 중 종료되었습니다.' }
            if ($client.MainWindowTitle -like 'WOD (*') { break }
            Start-Sleep -Milliseconds 300
        }
        if ($client.MainWindowTitle -notlike 'WOD (*') { throw '클라이언트 게임 창 준비 시간을 초과했습니다.' }
        Write-Host "클라이언트 게임 창 확인: $($client.MainWindowTitle)"
    }
    $result = @{ Configuration = $Configuration; ServersConnected = $true; ClientWindowChecked = (-not $ServersOnly); CheckedAtUtc = [DateTime]::UtcNow.ToString('O') }
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $script:RepoRoot "artifacts/logs/smoke-$Configuration.json") -Encoding utf8
    Write-Host "로컬 시작 smoke test: PASS ($Configuration)"
} finally {
    & "$PSScriptRoot/Stop-Local.ps1"
}
