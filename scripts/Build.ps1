param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [ValidateSet('All', 'Servers', 'LobbyServer', 'GameServer', 'Client')][string] $Module = 'All',
    [switch] $Rebuild
)
. "$PSScriptRoot/Development.ps1"
& "$PSScriptRoot/Check-Prerequisites.ps1"
$msbuild = Get-WodMSBuild
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
$logRoot = Join-Path $script:RepoRoot 'artifacts/logs'
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
if ($Module -eq 'All') {
    $buildPath = Join-Path $script:RepoRoot 'NewWod.slnx'
} elseif ($Module -eq 'Servers') {
    $buildPath = Join-Path $script:RepoRoot 'NewWod.Servers.slnf'
} else {
    $entry = @(Get-WodModules | Where-Object { $_.Name -eq $Module })[0]
    $buildPath = Join-Path $script:RepoRoot "$($entry.Directory)/$($entry.Project).vcxproj"
}
$log = Join-Path $logRoot "build-$Module-$Configuration.log"
& $msbuild $buildPath "/t:$target" "/p:Configuration=$Configuration" '/p:Platform=x64' '/nologo' '/clp:ErrorsOnly' '/fl' "/flp:logfile=$log;verbosity=normal"
if ($LASTEXITCODE -ne 0) { throw "빌드 실패: $Module. 로그: $log" }
Write-Host "빌드 완료: $Module $Configuration x64"
& "$PSScriptRoot/New-CompilationDatabase.ps1" -Configuration $Configuration
