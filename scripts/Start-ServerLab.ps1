param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [string] $Python = '',
    [ValidateRange(1024, 65535)][int] $Port = 8790,
    [switch] $ExistingServers,
    [switch] $Build,
    [switch] $UncapAdmission,
    [switch] $DisableMetrics,
    [string] $Directory = ''
)
. "$PSScriptRoot/Development.ps1"
$labState = Join-Path $script:RepoRoot '.runtime/server-lab.json'
if (Test-Path -LiteralPath $labState) { throw '기존 Server Lab 실행 기록이 있습니다. Stop-ServerLab.ps1을 먼저 실행하세요.' }
if (Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue) { throw "포트 $Port 사용 중" }
if ($Port -in @(8910, 8911)) { throw '게임 TCP 포트는 관측 화면에 사용할 수 없습니다.' }
$pythonCommand = Get-WodPython $Python
if ($Build) { & "$PSScriptRoot/Build.ps1" -Module Servers -Configuration $Configuration }
$labDirectory = if ($Directory) { [IO.Path]::GetFullPath($Directory) } else { Join-Path $script:RepoRoot 'artifacts/logs/server-lab' }
[IO.Directory]::CreateDirectory($labDirectory) | Out-Null
[IO.Directory]::CreateDirectory((Split-Path $labState)) | Out-Null
$startedServers = $false
$monitorProcess = $null
$previousDirectory = $env:WOD_METRICS_DIRECTORY
$previousAdmission = $env:WOD_PERF_UNCAPPED
try {
    if (-not $ExistingServers) {
        $env:WOD_METRICS_DIRECTORY = Join-Path $labDirectory 'metrics'
        if ($DisableMetrics) { $env:WOD_METRICS_DIRECTORY = $null }
        if ($UncapAdmission) { $env:WOD_PERF_UNCAPPED = '1' }
        & "$PSScriptRoot/Start-Local.ps1" -Configuration $Configuration -ServersOnly
        $startedServers = $true
    }
    $monitorScript = Join-Path $script:RepoRoot 'tools/server_lab/monitor.py'
    $monitorProcess = Start-Process -FilePath $pythonCommand -ArgumentList @('-u', ('"{0}"' -f $monitorScript), '--port', $Port, '--directory', ('"{0}"' -f $labDirectory)) -WorkingDirectory $script:RepoRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $labDirectory 'monitor.out.log') -RedirectStandardError (Join-Path $labDirectory 'monitor.err.log')
    Wait-WodListener $monitorProcess $Port
    @{ ProcessId = $monitorProcess.Id; Executable = $monitorProcess.Path; StartTimeUtc = $monitorProcess.StartTime.ToUniversalTime().ToString('O'); Port = $Port; StartedServers = $startedServers; Directory = $labDirectory } | ConvertTo-Json | Set-Content -LiteralPath $labState -Encoding utf8
    Write-Host "Server Lab 시작: http://127.0.0.1:$Port"
} catch {
    if ($monitorProcess -and -not $monitorProcess.HasExited) { Stop-Process -Id $monitorProcess.Id }
    if ($startedServers) { & "$PSScriptRoot/Stop-Local.ps1" }
    throw
} finally {
    $env:WOD_METRICS_DIRECTORY = $previousDirectory
    $env:WOD_PERF_UNCAPPED = $previousAdmission
}
