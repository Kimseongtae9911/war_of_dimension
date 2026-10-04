param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [ValidateSet('All', 'LobbyServer', 'GameServer', 'Client')][string] $Module = 'All',
    [switch] $Rebuild
)
. "$PSScriptRoot/Development.ps1"
& "$PSScriptRoot/Check-Prerequisites.ps1"
$msbuild = Get-WodMSBuild
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
$logRoot = Join-Path $script:RepoRoot 'artifacts/logs'
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
foreach ($entry in Get-WodModules) {
    if ($Module -ne 'All' -and $Module -ne $entry.Name) { continue }
    $solution = Join-Path $script:RepoRoot "$($entry.Directory)/$($entry.Project).sln"
    $log = Join-Path $logRoot "build-$($entry.Name)-$Configuration.log"
    & $msbuild $solution "/t:$target" "/p:Configuration=$Configuration" '/p:Platform=x64' '/nologo' '/clp:ErrorsOnly' '/fl' "/flp:logfile=$log;verbosity=normal"
    if ($LASTEXITCODE -ne 0) { throw "빌드 실패: $($entry.Name). 로그: $log" }
    Write-Host "빌드 완료: $($entry.Name) $Configuration x64"
}
& "$PSScriptRoot/New-CompilationDatabase.ps1" -Configuration $Configuration
