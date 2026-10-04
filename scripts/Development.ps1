Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:RepoRoot = Split-Path $PSScriptRoot -Parent

function Get-WodMSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer의 vswhere.exe가 필요합니다.' }
    $installation = & $vswhere -latest -products '*' -version '[18.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) { throw 'Visual Studio 2026의 Desktop development with C++ 워크로드가 필요합니다.' }
    return Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
}

function Get-WodModules {
    return @(
        @{ Name = 'LobbyServer'; Project = 'Lobby_Server'; Directory = 'Server/Lobby_Server' },
        @{ Name = 'GameServer'; Project = 'Game_Server'; Directory = 'Server/Game_Server' },
        @{ Name = 'Client'; Project = 'WarOfDimension'; Directory = 'Client/WarOfDimension' }
    )
}

function Get-WodExecutable([string] $Module, [string] $Configuration) {
    $entry = @(Get-WodModules | Where-Object { $_.Name -eq $Module })[0]
    return Join-Path $script:RepoRoot ('artifacts/bin/{0}/{1}/{2}.exe' -f $Configuration, $Module, $entry.Project)
}

function Wait-WodListener([System.Diagnostics.Process] $Process, [int] $Port) {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    while ([DateTime]::UtcNow -lt $deadline) {
        $Process.Refresh()
        if ($Process.HasExited) { throw ('프로세스 {0}가 종료되었습니다. artifacts/logs의 실행 로그를 확인하세요.' -f $Process.Id) }
        $listener = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue | Where-Object { $_.OwningProcess -eq $Process.Id }
        if ($listener) { return }
        Start-Sleep -Milliseconds 200
    }
    throw ('포트 {0} 대기 시간이 초과되었습니다.' -f $Port)
}
