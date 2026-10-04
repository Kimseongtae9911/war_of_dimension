param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [switch] $ServersOnly
)
. "$PSScriptRoot/Development.ps1"
$runtimeRoot = Join-Path $script:RepoRoot '.runtime'
$statePath = Join-Path $runtimeRoot 'processes.json'
if (Test-Path -LiteralPath $statePath) { throw '기존 실행 기록이 있습니다. 먼저 scripts/Stop-Local.ps1을 실행하세요.' }
foreach ($port in @(8910, 8911)) {
    if (Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue) { throw "포트 $port 를 다른 프로세스가 사용 중입니다." }
}
[System.IO.Directory]::CreateDirectory($runtimeRoot) | Out-Null
[System.IO.Directory]::CreateDirectory((Join-Path $script:RepoRoot 'artifacts/logs')) | Out-Null
$started = [System.Collections.Generic.List[object]]::new()
try {
    foreach ($entry in Get-WodModules) {
        if ($ServersOnly -and $entry.Name -eq 'Client') { continue }
        $executable = Get-WodExecutable $entry.Name $Configuration
        if (-not (Test-Path -LiteralPath $executable)) { throw "실행 파일이 없습니다. scripts/Build.ps1 -Configuration $Configuration 을 실행하세요." }
        $launch = @{ FilePath = $executable; WorkingDirectory = (Join-Path $script:RepoRoot $entry.Directory); PassThru = $true; WindowStyle = 'Hidden' }
        # 클라이언트는 사용자가 직접 조작하는 게임 창이며 서버만 백그라운드로 실행한다.
        if ($entry.Name -eq 'Client') { $launch.WindowStyle = 'Normal' }
        if ($entry.Name -ne 'Client') {
            $launch.RedirectStandardOutput = Join-Path $script:RepoRoot "artifacts/logs/run-$($entry.Name).out.log"
            $launch.RedirectStandardError = Join-Path $script:RepoRoot "artifacts/logs/run-$($entry.Name).err.log"
        }
        $process = Start-Process @launch
        $started.Add(@{ Module = $entry.Name; ProcessId = $process.Id; Executable = $executable; StartTimeUtc = $process.StartTime.ToUniversalTime().ToString('O') })
        if ($entry.Name -eq 'LobbyServer') { Wait-WodListener $process 8910 }
        if ($entry.Name -eq 'GameServer') { Wait-WodListener $process 8911 }
        Write-Host "시작 완료: $($entry.Name) (PID $($process.Id))"
    }
    $started.ToArray() | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $statePath -Encoding utf8
} catch {
    foreach ($record in $started) { Get-Process -Id $record.ProcessId -ErrorAction SilentlyContinue | Stop-Process -ErrorAction SilentlyContinue }
    throw
}
